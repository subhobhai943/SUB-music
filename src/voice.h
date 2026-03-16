#ifndef VOICE_H
#define VOICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct discord;

void voice_init(struct discord *client);
void voice_shutdown(void);

bool voice_join(uint64_t guild_id, uint64_t channel_id, char *error, size_t error_sz);
void voice_leave(uint64_t guild_id);
bool voice_send_opus_frame(uint64_t guild_id, const unsigned char *data, int len);

#endif
