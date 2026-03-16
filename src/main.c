#include "commands.h"
#include "discord_gateway.h"
#include "lavalink_client.h"
#include "queue.h"

#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>

static int load_config(const char *path,
                       char *token,
                       size_t token_sz,
                       char *ll_host,
                       size_t ll_host_sz,
                       int *ll_port,
                       char *ll_password,
                       size_t ll_password_sz,
                       char *intents,
                       size_t intents_sz) {
    json_error_t err;
    json_t *root = json_load_file(path, 0, &err);
    if (!root) {
        fprintf(stderr, "Failed to load config: %s\n", err.text);
        return -1;
    }

    json_t *discord = json_object_get(root, "discord");
    json_t *lavalink = json_object_get(root, "lavalink");

    const char *token_v = json_string_value(json_object_get(discord, "token"));
    const char *intents_v = json_string_value(json_object_get(discord, "intents"));
    const char *host_v = json_string_value(json_object_get(lavalink, "host"));
    const char *pass_v = json_string_value(json_object_get(lavalink, "password"));
    int port_v = (int)json_integer_value(json_object_get(lavalink, "port"));

    if (!token_v || !intents_v || !host_v || !pass_v || !port_v) {
        json_decref(root);
        return -1;
    }

    snprintf(token, token_sz, "%s", token_v);
    snprintf(intents, intents_sz, "%s", intents_v);
    snprintf(ll_host, ll_host_sz, "%s", host_v);
    snprintf(ll_password, ll_password_sz, "%s", pass_v);
    *ll_port = port_v;

    json_decref(root);
    return 0;
}

int main(void) {
    char token[256], ll_host[256], ll_password[128], intents[32];
    int ll_port = 2333;

    if (load_config("config/config.json", token, sizeof(token), ll_host, sizeof(ll_host), &ll_port, ll_password, sizeof(ll_password), intents, sizeof(intents)) != 0) {
        fprintf(stderr, "Invalid config/config.json\n");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    lavalink_client_t lavalink;
    lavalink_client_init(&lavalink, ll_host, ll_port, ll_password);

    queue_manager_t queues;
    queue_manager_init(&queues);

    command_context_t commands;
    commands_init(&commands, "s!", &lavalink, &queues);

    discord_gateway_config_t gateway_cfg;
    snprintf(gateway_cfg.token, sizeof(gateway_cfg.token), "%s", token);
    snprintf(gateway_cfg.intents_str, sizeof(gateway_cfg.intents_str), "%s", intents);

    discord_gateway_t gateway;
    if (discord_gateway_init(&gateway, &gateway_cfg, commands_handle_dispatch, &commands) != 0) {
        fprintf(stderr, "Failed to init Discord Gateway\n");
        return 1;
    }

    if (discord_gateway_connect(&gateway) != 0) {
        fprintf(stderr, "Failed to connect to Discord Gateway\n");
        discord_gateway_close(&gateway);
        return 1;
    }

    fprintf(stderr, "SUB Music started. Prefix: s!\n");
    discord_gateway_run(&gateway);

    discord_gateway_close(&gateway);
    commands_cleanup(&commands);
    queue_manager_free(&queues);
    curl_global_cleanup();
    return 0;
}
