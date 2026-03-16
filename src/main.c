#include "commands.h"
#include "music_player.h"
#include "voice.h"

#include <concord/discord.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char token[512];
  char prefix[16];
} BotConfig;

static bool parse_json_value(const char *json, const char *key, char *out,
                             size_t out_sz) {
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);

  const char *k = strstr(json, pattern);
  if (!k) return false;

  const char *colon = strchr(k, ':');
  if (!colon) return false;

  const char *start = strchr(colon, '"');
  if (!start) return false;
  start++;

  const char *end = strchr(start, '"');
  if (!end) return false;

  size_t len = (size_t)(end - start);
  if (len >= out_sz) len = out_sz - 1;
  memcpy(out, start, len);
  out[len] = '\0';
  return true;
}

static bool load_config(const char *path, BotConfig *cfg) {
  FILE *fp = fopen(path, "r");
  if (!fp) return false;

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size <= 0 || size > 8192) {
    fclose(fp);
    return false;
  }

  char *buf = calloc((size_t)size + 1, 1);
  if (!buf) {
    fclose(fp);
    return false;
  }

  fread(buf, 1, (size_t)size, fp);
  fclose(fp);

  bool ok = parse_json_value(buf, "token", cfg->token, sizeof(cfg->token));
  bool pfx = parse_json_value(buf, "prefix", cfg->prefix, sizeof(cfg->prefix));
  free(buf);

  return ok && pfx;
}

static void on_ready(struct discord *client, const struct discord_ready *event) {
  (void)client;
  printf("[READY] Logged in as %s#%s\n", event->user->username,
         event->user->discriminator);
}

static void on_message_create(struct discord *client,
                              const struct discord_message *event) {
  commands_handle_message(client, event);
}

int main(void) {
  BotConfig cfg = {0};

  if (!load_config("config/config.json", &cfg)) {
    fprintf(stderr, "Failed to load config/config.json\n");
    return 1;
  }

  commands_set_prefix(cfg.prefix);

  struct discord *client = discord_config_init("config/config.json");
  if (!client) {
    fprintf(stderr, "Failed to initialize Concord client\n");
    return 1;
  }

  voice_init(client);
  music_player_init();

  discord_set_on_ready(client, &on_ready);
  discord_set_on_message_create(client, &on_message_create);

  discord_run(client);

  music_player_shutdown();
  voice_shutdown();
  discord_cleanup(client);

  return 0;
}
