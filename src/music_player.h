#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_URL_LEN 1024
#define MAX_TITLE_LEN 256
#define MAX_QUEUE_SONGS 100

typedef struct {
  char url[MAX_URL_LEN];
  char title[MAX_TITLE_LEN];
  bool is_search;
} Song;

typedef struct {
  Song songs[MAX_QUEUE_SONGS];
  int front;
  int rear;
  int size;
} Queue;

void music_player_init(void);
void music_player_shutdown(void);

bool music_enqueue(uint64_t guild_id, uint64_t voice_channel_id, const Song *song,
                  char *status, size_t status_sz);
void music_skip(uint64_t guild_id, char *status, size_t status_sz);
void music_stop(uint64_t guild_id, char *status, size_t status_sz);
void music_leave(uint64_t guild_id, char *status, size_t status_sz);

#endif
