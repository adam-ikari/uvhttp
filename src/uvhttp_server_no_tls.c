#include "uvhttp_server_simple.h"
#include "uvhttp_request_simple.h"
#include "uvhttp_response_simple.h"
#include "uvhttp_router_simple.h"
#include "uvhttp_connection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct uvhttp_server* uvhttp_server_new(void* loop) {
    struct uvhttp_server* server = malloc(sizeof(struct uvhttp_server));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(struct uvhttp_server));
    
    server->loop = loop;
    server->is_listening = 0;
    server->active_connections = 0;
    
    return server;
}

void uvhttp_server_free(struct uvhttp_server* server) {
    if (server->router) {
        uvhttp_router_free(server->router);
    }
    free(server);
}

int uvhttp_server_listen(struct uvhttp_server* server, const char* host, int port) {
    // 简化版本只设置标记
    server->is_listening = 1;
    printf("Server listening on %s:%d\n", host, port);
    return 0;
}

void uvhttp_server_set_handler(struct uvhttp_server* server, uvhttp_request_handler_t handler) {
    server->handler = handler;
}

void uvhttp_server_stop(struct uvhttp_server* server) {
    if (server->is_listening) {
        server->is_listening = 0;
        printf("Server stopped\n");
    }
}