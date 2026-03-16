#include "discord_gateway.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GATEWAY_HOST "gateway.discord.gg"
#define GATEWAY_PATH "/?v=10&encoding=json"

static int callback_discord(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    (void)user;
    discord_gateway_t *gw = (discord_gateway_t *)lws_context_user(lws_get_context(wsi));

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            gw->wsi = wsi;
            fprintf(stderr, "Connected to Discord Gateway\n");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            char *msg = calloc(1, len + 1);
            if (!msg) break;
            memcpy(msg, in, len);

            json_error_t err;
            json_t *root = json_loads(msg, 0, &err);
            free(msg);
            if (!root) break;

            json_t *op = json_object_get(root, "op");
            json_t *d = json_object_get(root, "d");
            json_t *s = json_object_get(root, "s");
            json_t *t = json_object_get(root, "t");

            if (json_is_integer(s)) {
                gw->seq = json_integer_value(s);
            }

            if (json_is_integer(op) && json_integer_value(op) == 10) {
                json_t *interval = json_object_get(d, "heartbeat_interval");
                gw->heartbeat_interval_ms = json_is_integer(interval) ? (int)json_integer_value(interval) : 45000;
                gw->should_identify = 1;
                lws_callback_on_writable(wsi);
            } else if (json_is_string(t) && gw->dispatch_cb) {
                gw->dispatch_cb(gw, json_string_value(t), d, gw->userdata);
            }

            json_decref(root);
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            char out[16384] = {0};

            if (gw->should_identify) {
                snprintf(out, sizeof(out),
                         "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%s,\"properties\":{\"$os\":\"linux\",\"$browser\":\"submusic\",\"$device\":\"submusic\"}}}",
                         gw->config.token, gw->config.intents_str);
                gw->should_identify = 0;
            } else if (gw->has_pending_message) {
                snprintf(out, sizeof(out), "%s", gw->pending_message);
                gw->has_pending_message = 0;
            } else if (gw->heartbeat_interval_ms > 0) {
                snprintf(out, sizeof(out), "{\"op\":1,\"d\":%lld}", gw->seq >= 0 ? gw->seq : 0LL);
            }

            if (out[0]) {
                size_t out_len = strlen(out);
                unsigned char *buf = malloc(LWS_PRE + out_len);
                if (buf) {
                    memcpy(buf + LWS_PRE, out, out_len);
                    lws_write(wsi, buf + LWS_PRE, out_len, LWS_WRITE_TEXT);
                    free(buf);
                }
            }

            if (gw->heartbeat_interval_ms > 0) {
                lws_set_timer_usecs(wsi, (unsigned int)gw->heartbeat_interval_ms * 1000);
            }
            break;
        }

        case LWS_CALLBACK_TIMER:
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "Gateway connection error: %s\n", in ? (char *)in : "unknown");
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            fprintf(stderr, "Gateway closed\n");
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"discord-protocol", callback_discord, 0, 16384, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0}
};

int discord_gateway_init(discord_gateway_t *gw, const discord_gateway_config_t *cfg, discord_dispatch_cb cb, void *userdata) {
    memset(gw, 0, sizeof(*gw));
    gw->seq = -1;
    gw->dispatch_cb = cb;
    gw->userdata = userdata;
    snprintf(gw->config.token, sizeof(gw->config.token), "%s", cfg->token);
    snprintf(gw->config.intents_str, sizeof(gw->config.intents_str), "%s", cfg->intents_str);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.user = gw;

    gw->context = lws_create_context(&info);
    return gw->context ? 0 : -1;
}

int discord_gateway_connect(discord_gateway_t *gw) {
    struct lws_client_connect_info i;
    memset(&i, 0, sizeof(i));
    i.context = gw->context;
    i.address = GATEWAY_HOST;
    i.port = 443;
    i.path = GATEWAY_PATH;
    i.host = i.address;
    i.origin = i.address;
    i.protocol = protocols[0].name;
    i.ssl_connection = LCCSCF_USE_SSL;

    gw->wsi = lws_client_connect_via_info(&i);
    return gw->wsi ? 0 : -1;
}

int discord_gateway_send_text(discord_gateway_t *gw, const char *text) {
    snprintf(gw->pending_message, sizeof(gw->pending_message), "%s", text);
    gw->has_pending_message = 1;
    if (gw->wsi) lws_callback_on_writable(gw->wsi);
    return 0;
}

int discord_gateway_send_message(discord_gateway_t *gw, const char *channel_id, const char *content) {
    (void)gw;
    (void)channel_id;
    (void)content;
    return 0;
}

int discord_gateway_update_voice_state(discord_gateway_t *gw, const char *guild_id, const char *channel_id) {
    char payload[1024];
    if (channel_id) {
        snprintf(payload, sizeof(payload),
                 "{\"op\":4,\"d\":{\"guild_id\":\"%s\",\"channel_id\":\"%s\",\"self_mute\":false,\"self_deaf\":true}}",
                 guild_id, channel_id);
    } else {
        snprintf(payload, sizeof(payload),
                 "{\"op\":4,\"d\":{\"guild_id\":\"%s\",\"channel_id\":null,\"self_mute\":false,\"self_deaf\":true}}",
                 guild_id);
    }
    return discord_gateway_send_text(gw, payload);
}

int discord_gateway_run(discord_gateway_t *gw) {
    while (lws_service(gw->context, 500) >= 0) {
    }
    return 0;
}

void discord_gateway_close(discord_gateway_t *gw) {
    if (gw->context) lws_context_destroy(gw->context);
    gw->context = NULL;
}
