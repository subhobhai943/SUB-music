#ifndef COMMANDS_H
#define COMMANDS_H

#include "discord_gateway.h"
#include "lavalink_client.h"
#include "queue.h"
#include <jansson.h>

typedef struct voice_state_entry {
    char guild_id[32];
    char user_id[32];
    char channel_id[32];
    struct voice_state_entry *next;
} voice_state_entry_t;

typedef struct {
    const char *prefix;
    const char *bot_user_id;
    lavalink_client_t *lavalink;
    queue_manager_t *queues;
    voice_state_entry_t *voice_states;
} command_context_t;

void commands_init(command_context_t *ctx, const char *prefix, lavalink_client_t *lavalink, queue_manager_t *queues);
void commands_set_bot_user(command_context_t *ctx, const char *bot_user_id);
void commands_handle_dispatch(discord_gateway_t *gw, const char *event_name, json_t *payload, void *userdata);
void commands_cleanup(command_context_t *ctx);

#endif
