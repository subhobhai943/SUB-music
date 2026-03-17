#ifndef DISCORD_GATEWAY_H
#define DISCORD_GATEWAY_H

#include <jansson.h>
#include <libwebsockets.h>

typedef struct discord_gateway discord_gateway_t;

typedef void (*discord_dispatch_cb)(discord_gateway_t *gw, const char *event_name, json_t *payload, void *userdata);

typedef struct {
    char token[256];
    char intents_str[32];
} discord_gateway_config_t;

struct discord_gateway {
    struct lws_context *context;
    struct lws *wsi;
    discord_gateway_config_t config;
    int heartbeat_interval_ms;
    int should_identify;
    long long seq;
    char session_id[128];
    char pending_message[16384];
    int has_pending_message;

    discord_dispatch_cb dispatch_cb;
    void *userdata;
};

int discord_gateway_init(discord_gateway_t *gw, const discord_gateway_config_t *cfg, discord_dispatch_cb cb, void *userdata);
int discord_gateway_connect(discord_gateway_t *gw);
int discord_gateway_run(discord_gateway_t *gw);
void discord_gateway_close(discord_gateway_t *gw);
int discord_gateway_send_text(discord_gateway_t *gw, const char *text);
int discord_gateway_send_message(discord_gateway_t *gw, const char *channel_id, const char *content);
int discord_gateway_update_voice_state(discord_gateway_t *gw, const char *guild_id, const char *channel_id);

#endif
