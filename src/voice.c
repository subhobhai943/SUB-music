#include "voice.h"

#include <concord/discord.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Global discord client pointer */
static struct discord *g_client = NULL;

/**
 * Voice state for bot's connection to a voice channel.
 * This tracks whether the bot is connected and to which channel.
 */
typedef struct {
    uint64_t guild_id;
    uint64_t channel_id;
    bool connected;
} VoiceState;

/* Maximum number of guilds we can track */
#define MAX_GUILDS 32

/* Voice states for each guild (bot's connection state) */
static VoiceState voice_states[MAX_GUILDS];

/**
 * User voice state tracking.
 * This replaces the old member->voice->channel_id approach.
 * We track which voice channel each user is in by listening to
 * VOICE_STATE_UPDATE events.
 */
#define MAX_USER_STATES 256
static struct user_voice_state user_voice_states[MAX_USER_STATES];

/**
 * Find voice state for a guild.
 */
static VoiceState *find_voice_state(uint64_t guild_id) {
    for (int i = 0; i < MAX_GUILDS; ++i) {
        if (voice_states[i].guild_id == guild_id)
            return &voice_states[i];
    }
    /* Find empty slot */
    for (int i = 0; i < MAX_GUILDS; ++i) {
        if (voice_states[i].guild_id == 0) {
            voice_states[i].guild_id = guild_id;
            return &voice_states[i];
        }
    }
    return NULL;
}

/**
 * Find user voice state entry by user_id and guild_id.
 */
static struct user_voice_state *find_user_state(uint64_t guild_id, uint64_t user_id) {
    for (int i = 0; i < MAX_USER_STATES; ++i) {
        if (user_voice_states[i].guild_id == guild_id &&
            user_voice_states[i].user_id == user_id) {
            return &user_voice_states[i];
        }
    }
    return NULL;
}

/**
 * Find empty slot for new user voice state.
 */
static struct user_voice_state *find_empty_user_state(void) {
    for (int i = 0; i < MAX_USER_STATES; ++i) {
        if (user_voice_states[i].user_id == 0)
            return &user_voice_states[i];
    }
    return NULL;
}

void voice_init(struct discord *client) {
    g_client = client;
    memset(voice_states, 0, sizeof(voice_states));
    memset(user_voice_states, 0, sizeof(user_voice_states));
}

void voice_shutdown(void) {
    g_client = NULL;
}

/**
 * Get the voice channel ID for a user in a guild.
 * This replaces the old member->voice->channel_id approach.
 *
 * FIX: Changed from member->voice->channel_id to using our tracked
 * voice state map. The new Concord API does not expose voice state
 * through the message event's member struct.
 */
uint64_t voice_get_user_channel(uint64_t guild_id, uint64_t user_id) {
    struct user_voice_state *state = find_user_state(guild_id, user_id);
    if (state && state->channel_id != 0) {
        return state->channel_id;
    }
    return 0;
}

/**
 * Handle VOICE_STATE_UPDATE events.
 * This is called when a user's voice state changes.
 * We track the user's channel to know where they are.
 *
 * FIX: This replaces the old approach of accessing member->voice->channel_id
 * directly from the message event. The new Concord API requires us to
 * listen for VOICE_STATE_UPDATE events and maintain our own tracking.
 */
void voice_on_voice_state_update(struct discord *client,
                                  const struct discord_voice_state *event) {
    (void)client;

    if (!event || !event->guild_id || !event->user_id)
        return;

    uint64_t guild_id = event->guild_id;
    uint64_t user_id = event->user_id;
    uint64_t channel_id = event->channel_id;

    /* Find existing or create new user state */
    struct user_voice_state *state = find_user_state(guild_id, user_id);

    if (!state) {
        /* Try to find empty slot */
        state = find_empty_user_state();
    }

    if (state) {
        state->guild_id = guild_id;
        state->user_id = user_id;
        state->channel_id = channel_id;

        printf("[VOICE] User %" PRIu64 " state updated: channel=%" PRIu64 "\n",
               user_id, channel_id);
    }
}

bool voice_join(uint64_t guild_id, uint64_t channel_id, char *error, size_t error_sz) {
    VoiceState *state = find_voice_state(guild_id);
    if (!state) {
        snprintf(error, error_sz, "Voice state table is full");
        return false;
    }

    state->channel_id = channel_id;
    state->connected = true;

    /**
     * FIX: discord_update_voice_state() now takes a struct parameter
     * that includes guild_id, channel_id, self_mute, and self_deaf.
     * 
     * Old (incorrect) API:
     *   discord_update_voice_state(g_client, guild_id, &params, NULL)
     *
     * New (correct) API:
     *   discord_update_voice_state(g_client, &params)
     *
     * The guild_id is now part of the params struct.
     */
    struct discord_update_voice_state params = {
        .guild_id = guild_id,
        .channel_id = channel_id,
        .self_mute = false,
        .self_deaf = false,
    };

    if (g_client) {
        discord_update_voice_state(g_client, &params);
    }

    /* FIX: Use PRIu64 instead of %lu for uint64_t */
    printf("[VOICE] Joined guild=%" PRIu64 " channel=%" PRIu64 "\n", guild_id, channel_id);
    return true;
}

void voice_leave(uint64_t guild_id) {
    VoiceState *state = find_voice_state(guild_id);
    if (!state) return;

    state->connected = false;
    state->channel_id = 0;

    /**
     * FIX: discord_update_voice_state() with channel_id = 0 to disconnect.
     * Uses the new API with struct parameter.
     */
    struct discord_update_voice_state params = {
        .guild_id = guild_id,
        .channel_id = 0,
        .self_mute = false,
        .self_deaf = false,
    };

    if (g_client) {
        discord_update_voice_state(g_client, &params);
    }

    /* FIX: Use PRIu64 instead of %lu for uint64_t */
    printf("[VOICE] Left guild=%" PRIu64 "\n", guild_id);
}

bool voice_send_opus_frame(uint64_t guild_id, const unsigned char *data, int len) {
    (void)data;
    (void)len;
    VoiceState *state = find_voice_state(guild_id);
    if (!state || !state->connected) return false;

    /*
     * Concord voice UDP packet send API differs by version.
     * Integrate here with discord_voice_send_audio() or equivalent for your release.
     */
    return true;
}
