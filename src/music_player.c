#include "music_player.h"

#include "voice.h"
#include "youtube.h"

#include <opus/opus.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  uint64_t guild_id;
  uint64_t voice_channel_id;
  Queue queue;
  bool running;
  bool stop_requested;
  bool skip_requested;
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} GuildPlayer;

#define MAX_GUILD_PLAYERS 16
static GuildPlayer players[MAX_GUILD_PLAYERS];

static GuildPlayer *get_player(uint64_t guild_id) {
  for (int i = 0; i < MAX_GUILD_PLAYERS; ++i) {
    if (players[i].guild_id == guild_id) return &players[i];
  }
  for (int i = 0; i < MAX_GUILD_PLAYERS; ++i) {
    if (players[i].guild_id == 0) {
      players[i].guild_id = guild_id;
      players[i].queue.front = 0;
      players[i].queue.rear = -1;
      players[i].queue.size = 0;
      pthread_mutex_init(&players[i].mutex, NULL);
      pthread_cond_init(&players[i].cond, NULL);
      return &players[i];
    }
  }
  return NULL;
}

static bool queue_push(Queue *q, const Song *song) {
  if (q->size >= MAX_QUEUE_SONGS) return false;
  q->rear = (q->rear + 1) % MAX_QUEUE_SONGS;
  q->songs[q->rear] = *song;
  q->size++;
  return true;
}

static bool queue_pop(Queue *q, Song *song) {
  if (q->size == 0) return false;
  *song = q->songs[q->front];
  q->front = (q->front + 1) % MAX_QUEUE_SONGS;
  q->size--;
  return true;
}

static void queue_clear(Queue *q) {
  q->front = 0;
  q->rear = -1;
  q->size = 0;
}

static FILE *open_pcm_pipe(const char *resolved_url) {
  char cmd[8192];
  snprintf(cmd, sizeof(cmd),
           "yt-dlp -f bestaudio --no-playlist -o - '%s' 2>/dev/null | "
           "ffmpeg -loglevel error -i pipe:0 -f s16le -ar 48000 -ac 2 pipe:1 2>/dev/null",
           resolved_url);
  return popen(cmd, "r");
}

static bool play_song(GuildPlayer *player, Song *song) {
  char resolved_url[MAX_URL_LEN] = {0};
  char resolved_title[MAX_TITLE_LEN] = {0};
  char error[256] = {0};

  if (!youtube_resolve(song->url, song->is_search, resolved_url, sizeof(resolved_url),
                       resolved_title, sizeof(resolved_title), error,
                       sizeof(error))) {
    printf("[ERROR] youtube_resolve failed: %s\n", error);
    return false;
  }

  printf("[MUSIC] Now playing: %s\n", resolved_title);

  FILE *pcm = open_pcm_pipe(resolved_url);
  if (!pcm) {
    printf("[ERROR] Failed to start audio pipeline\n");
    return false;
  }

  int opus_err = 0;
  OpusEncoder *enc = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &opus_err);
  if (!enc || opus_err != OPUS_OK) {
    pclose(pcm);
    printf("[ERROR] Opus encoder init failed\n");
    return false;
  }

  short pcm_frame[960 * 2];
  unsigned char opus_data[4000];

  while (1) {
    pthread_mutex_lock(&player->mutex);
    bool stop = player->stop_requested;
    bool skip = player->skip_requested;
    if (skip) player->skip_requested = false;
    pthread_mutex_unlock(&player->mutex);

    if (stop || skip) break;

    size_t read_count = fread(pcm_frame, sizeof(short), 960 * 2, pcm);
    if (read_count < 960 * 2) {
      if (feof(pcm)) break;
      if (ferror(pcm)) break;
    }

    int len = opus_encode(enc, pcm_frame, 960, opus_data, (opus_int32)sizeof(opus_data));
    if (len < 0) {
      printf("[ERROR] opus_encode failed\n");
      break;
    }

    if (!voice_send_opus_frame(player->guild_id, opus_data, len)) {
      usleep(20 * 1000);
      continue;
    }

    usleep(20 * 1000);
  }

  opus_encoder_destroy(enc);
  pclose(pcm);
  return true;
}

static void *player_thread(void *arg) {
  GuildPlayer *player = (GuildPlayer *)arg;

  while (1) {
    Song song;

    pthread_mutex_lock(&player->mutex);
    while (player->queue.size == 0 && !player->stop_requested) {
      pthread_cond_wait(&player->cond, &player->mutex);
    }

    if (player->stop_requested) {
      player->running = false;
      pthread_mutex_unlock(&player->mutex);
      break;
    }

    bool ok = queue_pop(&player->queue, &song);
    pthread_mutex_unlock(&player->mutex);

    if (!ok) continue;
    play_song(player, &song);
  }

  return NULL;
}

void music_player_init(void) { memset(players, 0, sizeof(players)); }

void music_player_shutdown(void) {
  for (int i = 0; i < MAX_GUILD_PLAYERS; ++i) {
    if (players[i].guild_id == 0) continue;
    pthread_mutex_lock(&players[i].mutex);
    players[i].stop_requested = true;
    pthread_cond_signal(&players[i].cond);
    pthread_mutex_unlock(&players[i].mutex);
    if (players[i].running) pthread_join(players[i].thread, NULL);
    pthread_mutex_destroy(&players[i].mutex);
    pthread_cond_destroy(&players[i].cond);
  }
}

bool music_enqueue(uint64_t guild_id, uint64_t voice_channel_id, const Song *song,
                  char *status, size_t status_sz) {
  GuildPlayer *player = get_player(guild_id);
  if (!player) {
    snprintf(status, status_sz, "No available guild player slot.");
    return false;
  }

  pthread_mutex_lock(&player->mutex);
  if (!queue_push(&player->queue, song)) {
    pthread_mutex_unlock(&player->mutex);
    snprintf(status, status_sz, "Queue is full (max %d songs).", MAX_QUEUE_SONGS);
    return false;
  }

  player->voice_channel_id = voice_channel_id;
  player->stop_requested = false;

  if (!player->running) {
    player->running = true;
    if (pthread_create(&player->thread, NULL, player_thread, player) != 0) {
      player->running = false;
      pthread_mutex_unlock(&player->mutex);
      snprintf(status, status_sz, "Failed to start music thread.");
      return false;
    }
  }

  pthread_cond_signal(&player->cond);
  int q_size = player->queue.size;
  pthread_mutex_unlock(&player->mutex);

  snprintf(status, status_sz, "Queued: %s (queue size: %d)", song->url, q_size);
  return true;
}

void music_skip(uint64_t guild_id, char *status, size_t status_sz) {
  GuildPlayer *player = get_player(guild_id);
  if (!player || !player->running) {
    snprintf(status, status_sz, "Nothing is currently playing.");
    return;
  }
  pthread_mutex_lock(&player->mutex);
  player->skip_requested = true;
  pthread_mutex_unlock(&player->mutex);
  snprintf(status, status_sz, "Skipped current song.");
}

void music_stop(uint64_t guild_id, char *status, size_t status_sz) {
  GuildPlayer *player = get_player(guild_id);
  if (!player) {
    snprintf(status, status_sz, "Nothing to stop.");
    return;
  }
  pthread_mutex_lock(&player->mutex);
  queue_clear(&player->queue);
  player->stop_requested = true;
  pthread_cond_signal(&player->cond);
  pthread_mutex_unlock(&player->mutex);

  if (player->running) {
    pthread_join(player->thread, NULL);
    player->running = false;
  }

  snprintf(status, status_sz, "Playback stopped and queue cleared.");
}

void music_leave(uint64_t guild_id, char *status, size_t status_sz) {
  char tmp[128];
  music_stop(guild_id, tmp, sizeof(tmp));
  voice_leave(guild_id);
  snprintf(status, status_sz, "Left voice channel and stopped playback.");
}
