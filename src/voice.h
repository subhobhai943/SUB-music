#ifndef VOICE_H
#define VOICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct discord;
struct discord_voice_state;

/**
 * Voice state for tracking users in voice channels.
 * This replaces the old member->voice->channel_id approach.
 */
struct user_voice_state {
    uint64_t user_id;      /**< Discord user ID */
    uint64_t channel_id;   /**< Voice channel ID the user is in */
    uint64_t guild_id;     /**< Guild ID for this voice state */
};

/**
 * Initialize the voice module.
 * Sets up the discord client for voice operations.
 *
 * @param client Pointer to the Concord discord client
 */
void voice_init(struct discord *client);

/**
 * Shutdown the voice module.
 * Cleans up resources.
 */
void voice_shutdown(void);

/**
 * Join a voice channel.
 * Uses the new discord_update_voice_state() API.
 *
 * @param guild_id   The guild ID
 * @param channel_id The channel ID to join
 * @param error      Error message buffer (if returning false)
 * @param error_sz   Size of error buffer
 * @return true on success, false on failure
 */
bool voice_join(uint64_t guild_id, uint64_t channel_id, char *error, size_t error_sz);

/**
 * Leave the current voice channel.
 * Uses the new discord_update_voice_state() API.
 *
 * @param guild_id The guild ID
 */
void voice_leave(uint64_t guild_id);

/**
 * Send an Opus audio frame to the voice channel.
 *
 * @param guild_id   The guild ID
 * @param data       Opus audio data
 * @param len        Length of audio data
 * @return true on success, false on failure
 */
bool voice_send_opus_frame(uint64_t guild_id, const unsigned char *data, int len);

/**
 * Handle VOICE_STATE_UPDATE events.
 * This is called when a user's voice state changes.
 * We track the user's channel to know where they are.
 *
 * @param client Pointer to the Concord discord client
 * @param event  Voice state update event
 */
void voice_on_voice_state_update(struct discord *client,
                                  const struct discord_voice_state *event);

/**
 * Get the voice channel ID for a user in a guild.
 * This replaces the old member->voice->channel_id approach.
 *
 * @param guild_id The guild ID
 * @param user_id  The user ID to look up
 * @return The channel ID, or 0 if not in a voice channel
 */
uint64_t voice_get_user_channel(uint64_t guild_id, uint64_t user_id);

#endif /* VOICE_H */
