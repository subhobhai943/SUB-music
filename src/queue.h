#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef struct track_item {
    char *track;
    char *title;
    struct track_item *next;
} track_item_t;

typedef struct guild_queue {
    char guild_id[32];
    track_item_t *head;
    track_item_t *tail;
    struct guild_queue *next;
} guild_queue_t;

typedef struct {
    guild_queue_t *guilds;
} queue_manager_t;

void queue_manager_init(queue_manager_t *manager);
void queue_manager_free(queue_manager_t *manager);
int queue_push(queue_manager_t *manager, const char *guild_id, const char *track, const char *title);
track_item_t *queue_pop(queue_manager_t *manager, const char *guild_id);
track_item_t *queue_peek(queue_manager_t *manager, const char *guild_id);
void queue_clear(queue_manager_t *manager, const char *guild_id);
size_t queue_length(queue_manager_t *manager, const char *guild_id);

#endif
