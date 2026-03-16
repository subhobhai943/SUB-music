#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

struct discord;
struct discord_message;

void commands_set_prefix(const char *prefix);
void commands_handle_message(struct discord *client,
                             const struct discord_message *event);

#endif
