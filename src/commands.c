#include "commands.h"

#include "music_player.h"
#include "voice.h"
#include "youtube.h"

#include <concord/discord.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static char g_prefix[16] = "s!";

/**
 * Send a text message to a channel.
 *
 * FIX: Added const qualifier to the text parameter.
 * The Concord API requires const char* for the content field.
 * This fixes "initialization discards const qualifier" warnings.
 */
static void send_text(struct discord *client, u64snowflake channel_id,
                      const char *text) {
    struct discord_create_message params = {
        .content = text
    };
    discord_create_message(client, channel_id, &params, NULL);
}

void commands_set_prefix(const char *prefix) {
    if (!prefix || !*prefix) return;
    snprintf(g_prefix, sizeof(g_prefix), "%s", prefix);
}

static void command_help(struct discord *client, const struct discord_message *event) {
    const char *help =
        "**SUB Music Commands**\n"
        "`s!p <song or link>`  → Play music\n"
        "`s!skip`              → Skip song\n"
        "`s!stop`              → Stop playback\n"
        "`s!leave`             → Leave voice channel\n"
        "`s!help`              → Show commands";
    send_text(client, event->channel_id, help);
}

/**
 * Get the voice channel ID for the command author.
 *
 * FIX: Replaced the old member->voice->channel_id approach.
 * 
 * Old (broken) code:
 *   if (!event->member || !event->member->voice) return 0;
 *   return event->member->voice->channel_id;
 *
 * The new Concord API does not expose voice state through the message
 * event's member struct. Instead, we track voice states by listening
 * to VOICE_STATE_UPDATE events and maintain our own mapping.
 */
static uint64_t get_voice_channel_for_user(const struct discord_message *event) {
    if (!event->guild_id || !event->author)
        return 0;

    uint64_t guild_id = event->guild_id;
    uint64_t user_id = event->author->id;

    return voice_get_user_channel(guild_id, user_id);
}

void commands_handle_message(struct discord *client,
                             const struct discord_message *event) {
    if (!event || !event->content || !event->author) return;
    if (event->author->bot) return;

    size_t pfx = strlen(g_prefix);
    if (strncmp(event->content, g_prefix, pfx) != 0) return;

    const char *input = event->content + pfx;

    if (strncmp(input, "help", 4) == 0) {
        command_help(client, event);
        return;
    }

    if (strncmp(input, "skip", 4) == 0) {
        char status[256];
        music_skip(event->guild_id, status, sizeof(status));
        send_text(client, event->channel_id, status);
        return;
    }

    if (strncmp(input, "stop", 4) == 0) {
        char status[256];
        music_stop(event->guild_id, status, sizeof(status));
        send_text(client, event->channel_id, status);
        return;
    }

    if (strncmp(input, "leave", 5) == 0) {
        char status[256];
        music_leave(event->guild_id, status, sizeof(status));
        send_text(client, event->channel_id, status);
        return;
    }

    if (strncmp(input, "p ", 2) == 0) {
        const char *query = input + 2;
        if (!*query) {
            send_text(client, event->channel_id, "Usage: s!p <youtube_link_or_song_name>");
            return;
        }

        /**
         * FIX: Use voice_get_user_channel() instead of member->voice->channel_id.
         * 
         * This function looks up the user's voice channel from our
         * tracked voice states (populated via VOICE_STATE_UPDATE events).
         */
        uint64_t voice_channel_id = get_voice_channel_for_user(event);
        if (!voice_channel_id) {
            send_text(client, event->channel_id,
                     "Join a voice channel first, then use `s!p ...`");
            return;
        }

        char err[128];
        if (!voice_join(event->guild_id, voice_channel_id, err, sizeof(err))) {
            send_text(client, event->channel_id, err);
            return;
        }

        Song song = {0};
        snprintf(song.url, sizeof(song.url), "%s", query);
        song.is_search = !youtube_is_url(query);

        char status[256];
        if (!music_enqueue(event->guild_id, voice_channel_id, &song, status,
                          sizeof(status))) {
            send_text(client, event->channel_id, status);
            return;
        }

        send_text(client, event->channel_id, status);
        return;
    }

    command_help(client, event);
}
