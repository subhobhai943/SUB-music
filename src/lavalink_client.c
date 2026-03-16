#include "lavalink_client.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *ptr;
    size_t len;
} response_buffer_t;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    response_buffer_t *buf = (response_buffer_t *)userp;
    char *new_ptr = realloc(buf->ptr, buf->len + total + 1);
    if (!new_ptr) {
        return 0;
    }
    buf->ptr = new_ptr;
    memcpy(buf->ptr + buf->len, contents, total);
    buf->len += total;
    buf->ptr[buf->len] = '\0';
    return total;
}

void lavalink_client_init(lavalink_client_t *client, const char *host, int port, const char *password) {
    snprintf(client->host, sizeof(client->host), "%s", host);
    client->port = port;
    snprintf(client->password, sizeof(client->password), "%s", password);
    client->session_id[0] = '\0';
}

void lavalink_client_set_session(lavalink_client_t *client, const char *session_id) {
    snprintf(client->session_id, sizeof(client->session_id), "%s", session_id ? session_id : "");
}

static int perform_json_request(CURL *curl, response_buffer_t *resp, long *status_code) {
    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(code));
        return -1;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status_code);
    return 0;
}

int lavalink_search_track(lavalink_client_t *client, const char *query, lavalink_track_result_t *out) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    out->found = 0;
    char *escaped = curl_easy_escape(curl, query, 0);
    if (!escaped) {
        curl_easy_cleanup(curl);
        return -1;
    }

    char url[2048];
    snprintf(url, sizeof(url), "http://%s:%d/v4/loadtracks?identifier=ytsearch:%s", client->host, client->port, escaped);

    struct curl_slist *headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: %s", client->password);
    headers = curl_slist_append(headers, auth_header);

    response_buffer_t resp = { .ptr = calloc(1, 1), .len = 0 };
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    long status = 0;
    int rc = perform_json_request(curl, &resp, &status);
    if (rc == 0 && status >= 200 && status < 300) {
        json_error_t err;
        json_t *root = json_loads(resp.ptr, 0, &err);
        if (root) {
            json_t *data = json_object_get(root, "data");
            if (json_is_array(data) && json_array_size(data) > 0) {
                json_t *first = json_array_get(data, 0);
                json_t *encoded = json_object_get(first, "encoded");
                json_t *info = json_object_get(first, "info");
                json_t *title = info ? json_object_get(info, "title") : NULL;
                if (json_is_string(encoded)) {
                    snprintf(out->track, sizeof(out->track), "%s", json_string_value(encoded));
                    snprintf(out->title, sizeof(out->title), "%s", json_is_string(title) ? json_string_value(title) : "Unknown title");
                    out->found = 1;
                }
            }
            json_decref(root);
        }
    }

    curl_free(escaped);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(resp.ptr);
    return out->found ? 0 : -1;
}

static int lavalink_patch_player(lavalink_client_t *client, const char *guild_id, json_t *payload) {
    if (client->session_id[0] == '\0') {
        fprintf(stderr, "Lavalink session ID missing.\n");
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/v4/sessions/%s/players/%s", client->host, client->port, client->session_id, guild_id);

    char *json_body = json_dumps(payload, JSON_COMPACT);
    if (!json_body) {
        curl_easy_cleanup(curl);
        return -1;
    }

    struct curl_slist *headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: %s", client->password);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");

    response_buffer_t resp = { .ptr = calloc(1, 1), .len = 0 };
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    long status = 0;
    int rc = perform_json_request(curl, &resp, &status);
    if (rc == 0 && !(status >= 200 && status < 300)) {
        rc = -1;
    }

    if (rc != 0) {
        fprintf(stderr, "Lavalink PATCH failed: %s\n", resp.ptr ? resp.ptr : "<empty>");
    }

    free(resp.ptr);
    curl_slist_free_all(headers);
    free(json_body);
    curl_easy_cleanup(curl);
    return rc;
}

int lavalink_play_track(lavalink_client_t *client, const char *guild_id, const char *track) {
    json_t *payload = json_pack("{s:{s:s}, s:b}", "track", "encoded", track, "noReplace", 0);
    int rc = lavalink_patch_player(client, guild_id, payload);
    json_decref(payload);
    return rc;
}

int lavalink_stop(lavalink_client_t *client, const char *guild_id) {
    json_t *payload = json_pack("{s:n}", "track");
    int rc = lavalink_patch_player(client, guild_id, payload);
    json_decref(payload);
    return rc;
}

int lavalink_leave(lavalink_client_t *client, const char *guild_id) {
    json_t *payload = json_pack("{s:b}", "paused", 1);
    int rc = lavalink_patch_player(client, guild_id, payload);
    json_decref(payload);
    return rc;
}
