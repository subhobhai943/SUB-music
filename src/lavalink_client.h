#ifndef LAVALINK_CLIENT_H
#define LAVALINK_CLIENT_H

#include <jansson.h>

typedef struct {
    char host[256];
    int port;
    char password[128];
    char session_id[128];
} lavalink_client_t;

typedef struct {
    char track[8192];
    char title[512];
    int found;
} lavalink_track_result_t;

void lavalink_client_init(lavalink_client_t *client, const char *host, int port, const char *password);
void lavalink_client_set_session(lavalink_client_t *client, const char *session_id);
int lavalink_search_track(lavalink_client_t *client, const char *query, lavalink_track_result_t *out);
int lavalink_play_track(lavalink_client_t *client, const char *guild_id, const char *track);
int lavalink_stop(lavalink_client_t *client, const char *guild_id);
int lavalink_leave(lavalink_client_t *client, const char *guild_id);

#endif
