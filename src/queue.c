#include "queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static char *dupstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

static guild_queue_t *find_guild(queue_manager_t *manager, const char *guild_id, int create_if_missing) {
    guild_queue_t *cursor = manager->guilds;
    while (cursor) {
        if (strcmp(cursor->guild_id, guild_id) == 0) {
            return cursor;
        }
        cursor = cursor->next;
    }

    if (!create_if_missing) {
        return NULL;
    }

    guild_queue_t *node = calloc(1, sizeof(guild_queue_t));
    if (!node) {
        return NULL;
    }

    snprintf(node->guild_id, sizeof(node->guild_id), "%s", guild_id);
    node->next = manager->guilds;
    manager->guilds = node;
    return node;
}

void queue_manager_init(queue_manager_t *manager) {
    manager->guilds = NULL;
}

static void free_track(track_item_t *item) {
    if (!item) return;
    free(item->track);
    free(item->title);
    free(item);
}

void queue_manager_free(queue_manager_t *manager) {
    guild_queue_t *guild = manager->guilds;
    while (guild) {
        guild_queue_t *next = guild->next;
        queue_clear(manager, guild->guild_id);
        free(guild);
        guild = next;
    }
    manager->guilds = NULL;
}

int queue_push(queue_manager_t *manager, const char *guild_id, const char *track, const char *title) {
    guild_queue_t *guild = find_guild(manager, guild_id, 1);
    if (!guild) {
        return -1;
    }

    track_item_t *item = calloc(1, sizeof(track_item_t));
    if (!item) {
        return -1;
    }

    item->track = dupstr(track);
    item->title = dupstr(title ? title : "Unknown title");
    if (!item->track || !item->title) {
        free_track(item);
        return -1;
    }

    if (guild->tail) {
        guild->tail->next = item;
    } else {
        guild->head = item;
    }
    guild->tail = item;
    return 0;
}

track_item_t *queue_pop(queue_manager_t *manager, const char *guild_id) {
    guild_queue_t *guild = find_guild(manager, guild_id, 0);
    if (!guild || !guild->head) {
        return NULL;
    }

    track_item_t *item = guild->head;
    guild->head = item->next;
    if (!guild->head) {
        guild->tail = NULL;
    }
    item->next = NULL;
    return item;
}

track_item_t *queue_peek(queue_manager_t *manager, const char *guild_id) {
    guild_queue_t *guild = find_guild(manager, guild_id, 0);
    return guild ? guild->head : NULL;
}

void queue_clear(queue_manager_t *manager, const char *guild_id) {
    guild_queue_t *guild = find_guild(manager, guild_id, 0);
    if (!guild) {
        return;
    }

    track_item_t *item = guild->head;
    while (item) {
        track_item_t *next = item->next;
        free_track(item);
        item = next;
    }

    guild->head = NULL;
    guild->tail = NULL;
}

size_t queue_length(queue_manager_t *manager, const char *guild_id) {
    guild_queue_t *guild = find_guild(manager, guild_id, 0);
    size_t count = 0;
    track_item_t *cursor = guild ? guild->head : NULL;
    while (cursor) {
        count++;
        cursor = cursor->next;
    }
    return count;
}
