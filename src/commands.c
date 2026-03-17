#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dupstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

static voice_state_entry_t *find_voice(command_context_t *ctx, const char *guild_id, const char *user_id, int create) {
    voice_state_entry_t *cur = ctx->voice_states;
    while (cur) {
        if (strcmp(cur->guild_id, guild_id) == 0 && strcmp(cur->user_id, user_id) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    if (!create) return NULL;

    voice_state_entry_t *entry = calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    snprintf(entry->guild_id, sizeof(entry->guild_id), "%s", guild_id);
    snprintf(entry->user_id, sizeof(entry->user_id), "%s", user_id);
    entry->next = ctx->voice_states;
    ctx->voice_states = entry;
    return entry;
}

static void handle_voice_state_update(command_context_t *ctx, json_t *payload) {
    const char *guild_id = json_string_value(json_object_get(payload, "guild_id"));
    const char *channel_id = json_string_value(json_object_get(payload, "channel_id"));
    json_t *user = json_object_get(payload, "user_id");

    if (!guild_id || !json_is_string(user)) return;
    const char *user_id = json_string_value(user);

    voice_state_entry_t *entry = find_voice(ctx, guild_id, user_id, 1);
    if (!entry) return;
    snprintf(entry->channel_id, sizeof(entry->channel_id), "%s", channel_id ? channel_id : "");
}

static const char *user_voice_channel(command_context_t *ctx, const char *guild_id, const char *user_id) {
    voice_state_entry_t *entry = find_voice(ctx, guild_id, user_id, 0);
    if (!entry || entry->channel_id[0] == '\0') return NULL;
    return entry->channel_id;
}

static void send_text_feedback(const char *text) {
    fprintf(stderr, "[SUB Music] %s\n", text);
}

static void handle_play(command_context_t *ctx, const char *guild_id, const char *user_id, const char *arg) {
    if (!arg || arg[0] == '\0') {
        send_text_feedback("Usage: s!p <youtube link or search query>");
        return;
    }

    lavalink_track_result_t result;
    if (lavalink_search_track(ctx->lavalink, arg, &result) != 0) {
        send_text_feedback("No results found in Lavalink.");
        return;
    }

    if (queue_push(ctx->queues, guild_id, result.track, result.title) != 0) {
        send_text_feedback("Failed to queue track.");
        return;
    }

    const char *voice_channel = user_voice_channel(ctx, guild_id, user_id);
    if (voice_channel) {
        send_text_feedback("Joining user's voice channel...");
    }

    if (voice_channel) {
        /* Voice gateway update happens in caller with discord handle. */
    }

    track_item_t *current = queue_peek(ctx->queues, guild_id);
    if (current && lavalink_play_track(ctx->lavalink, guild_id, current->track) == 0) {
        send_text_feedback("Track sent to Lavalink player.");
    } else {
        send_text_feedback("Could not start playback on Lavalink.");
    }
}

static void handle_skip(command_context_t *ctx, const char *guild_id) {
    track_item_t *old = queue_pop(ctx->queues, guild_id);
    if (old) {
        free(old->track);
        free(old->title);
        free(old);
    }

    track_item_t *next = queue_peek(ctx->queues, guild_id);
    if (!next) {
        lavalink_stop(ctx->lavalink, guild_id);
        send_text_feedback("Queue empty. Playback stopped.");
        return;
    }

    if (lavalink_play_track(ctx->lavalink, guild_id, next->track) == 0) {
        send_text_feedback("Skipped to next song.");
    } else {
        send_text_feedback("Failed to play next song.");
    }
}

static void handle_stop(command_context_t *ctx, const char *guild_id) {
    queue_clear(ctx->queues, guild_id);
    if (lavalink_stop(ctx->lavalink, guild_id) == 0) {
        send_text_feedback("Playback stopped and queue cleared.");
    }
}

static void handle_leave(command_context_t *ctx, discord_gateway_t *gw, const char *guild_id) {
    queue_clear(ctx->queues, guild_id);
    lavalink_leave(ctx->lavalink, guild_id);
    discord_gateway_update_voice_state(gw, guild_id, NULL);
    send_text_feedback("Left voice channel.");
}

static void handle_help(void) {
    send_text_feedback("Commands: s!p <song|link>, s!skip, s!stop, s!leave, s!help");
}

void commands_init(command_context_t *ctx, const char *prefix, lavalink_client_t *lavalink, queue_manager_t *queues) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->prefix = prefix;
    ctx->lavalink = lavalink;
    ctx->queues = queues;
}

void commands_set_bot_user(command_context_t *ctx, const char *bot_user_id) {
    ctx->bot_user_id = bot_user_id;
}

void commands_handle_dispatch(discord_gateway_t *gw, const char *event_name, json_t *payload, void *userdata) {
    command_context_t *ctx = (command_context_t *)userdata;

    if (strcmp(event_name, "READY") == 0) {
        const char *session_id = json_string_value(json_object_get(payload, "session_id"));
        if (session_id) {
            lavalink_client_set_session(ctx->lavalink, session_id);
        }
        json_t *user = json_object_get(payload, "user");
        const char *user_id = user ? json_string_value(json_object_get(user, "id")) : NULL;
        if (user_id) {
            commands_set_bot_user(ctx, dupstr(user_id));
        }
        send_text_feedback("Gateway READY received.");
        return;
    }

    if (strcmp(event_name, "VOICE_STATE_UPDATE") == 0) {
        handle_voice_state_update(ctx, payload);
        return;
    }

    if (strcmp(event_name, "MESSAGE_CREATE") != 0) {
        return;
    }

    const char *content = json_string_value(json_object_get(payload, "content"));
    const char *guild_id = json_string_value(json_object_get(payload, "guild_id"));
    const char *channel_id = json_string_value(json_object_get(payload, "channel_id"));
    (void)channel_id;

    json_t *author = json_object_get(payload, "author");
    const char *author_id = author ? json_string_value(json_object_get(author, "id")) : NULL;
    if (!content || !guild_id || !author_id) {
        return;
    }

    if (ctx->bot_user_id && strcmp(author_id, ctx->bot_user_id) == 0) {
        return;
    }

    size_t prefix_len = strlen(ctx->prefix);
    if (strncmp(content, ctx->prefix, prefix_len) != 0) {
        return;
    }

    const char *cmdline = content + prefix_len;
    while (*cmdline == ' ') cmdline++;

    if (strncmp(cmdline, "p", 1) == 0 && (cmdline[1] == ' ' || cmdline[1] == '\0')) {
        const char *arg = cmdline + 1;
        while (*arg == ' ') arg++;

        const char *voice_channel = user_voice_channel(ctx, guild_id, author_id);
        if (voice_channel) {
            discord_gateway_update_voice_state(gw, guild_id, voice_channel);
        } else {
            send_text_feedback("User is not in a voice channel (or state not cached yet).");
        }
        handle_play(ctx, guild_id, author_id, arg);
    } else if (strcmp(cmdline, "skip") == 0) {
        handle_skip(ctx, guild_id);
    } else if (strcmp(cmdline, "stop") == 0) {
        handle_stop(ctx, guild_id);
    } else if (strcmp(cmdline, "leave") == 0) {
        handle_leave(ctx, gw, guild_id);
    } else if (strcmp(cmdline, "help") == 0) {
        handle_help();
    }
}

void commands_cleanup(command_context_t *ctx) {
    voice_state_entry_t *cursor = ctx->voice_states;
    while (cursor) {
        voice_state_entry_t *next = cursor->next;
        free(cursor);
        cursor = next;
    }
    ctx->voice_states = NULL;

    if (ctx->bot_user_id) {
        free((char *)ctx->bot_user_id);
        ctx->bot_user_id = NULL;
    }
}
