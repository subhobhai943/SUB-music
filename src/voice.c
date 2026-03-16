#include "voice.h"

#include <concord/discord.h>
#include <stdio.h>
#include <string.h>

static struct discord *g_client = NULL;

typedef struct {
  uint64_t guild_id;
  uint64_t channel_id;
  bool connected;
} VoiceState;

#define MAX_GUILDS 32
static VoiceState states[MAX_GUILDS];

static VoiceState *find_state(uint64_t guild_id) {
  for (int i = 0; i < MAX_GUILDS; ++i) {
    if (states[i].guild_id == guild_id) return &states[i];
  }
  for (int i = 0; i < MAX_GUILDS; ++i) {
    if (states[i].guild_id == 0) {
      states[i].guild_id = guild_id;
      return &states[i];
    }
  }
  return NULL;
}

void voice_init(struct discord *client) {
  g_client = client;
  memset(states, 0, sizeof(states));
}

void voice_shutdown(void) { g_client = NULL; }

bool voice_join(uint64_t guild_id, uint64_t channel_id, char *error, size_t error_sz) {
  VoiceState *state = find_state(guild_id);
  if (!state) {
    snprintf(error, error_sz, "Voice state table is full");
    return false;
  }

  state->channel_id = channel_id;
  state->connected = true;

  struct discord_update_voice_state params = {
      .channel_id = channel_id,
      .self_mute = false,
      .self_deaf = false,
  };

  if (g_client) {
    discord_update_voice_state(g_client, guild_id, &params, NULL);
  }

  printf("[VOICE] Joined guild=%lu channel=%lu\n", guild_id, channel_id);
  return true;
}

void voice_leave(uint64_t guild_id) {
  VoiceState *state = find_state(guild_id);
  if (!state) return;

  state->connected = false;
  state->channel_id = 0;

  struct discord_update_voice_state params = {
      .channel_id = 0,
      .self_mute = false,
      .self_deaf = false,
  };

  if (g_client) {
    discord_update_voice_state(g_client, guild_id, &params, NULL);
  }

  printf("[VOICE] Left guild=%lu\n", guild_id);
}

bool voice_send_opus_frame(uint64_t guild_id, const unsigned char *data, int len) {
  (void)data;
  (void)len;
  VoiceState *state = find_state(guild_id);
  if (!state || !state->connected) return false;

  /*
   * Concord voice UDP packet send API differs by version.
   * Integrate here with discord_voice_send_audio() or equivalent for your release.
   */
  return true;
}
