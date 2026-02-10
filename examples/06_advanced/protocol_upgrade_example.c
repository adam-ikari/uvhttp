/**
 * @file protocol_upgrade_example.c
 * @brief Protocol upgrade example program
 * 
 * This example demonstrates how to use UVHTTP protocol upgrade framework
 * to implement HTTP to custom protocol upgrade.
 * 
 * Example scenarios:
 * 1. Normal HTTP request processing
 * 2. IPPS protocol upgrade (simulating printer protocol)
 * 3. gRPC-Web protocol upgrade (simulating gRPC over HTTP)
 * 
 * @copyright Copyright (c) 2026
 * @license MIT License
 */

#include <uvhttp.h>
#include <uvhttp_protocol_upgrade.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ========== Function declarations ========== */

void on_transfer_to_ipps(uv_tcp_t* tcp_handle, int fd, void* user_data);
void on_ipps_connection_close(void* user_data);
void on_transfer_to_grpc(uv_tcp_t* tcp_handle, int fd, void* user_data);

/* ========== Simulate IPPS protocol ========== */

/**
 * @brief IPPS context
 */
typedef struct {
    uv_loop_t* loop;
    int connection_count;
} ipps_context_t;

/**
 * @brief IPPS connection
 */
typedef struct {
    int fd;
    char client_ip[INET6_ADDRSTRLEN];
    ipps_context_t* ctx;
} ipps_connection_t;

/**
 * @brief IPPS protocol detector
 */
static int ipps_protocol_detector(uvhttp_request_t* request,
                                  char* protocol_name,
                                  size_t protocol_name_len,
                                  const char* upgrade_header,
                                  const char* connection_header) {
    (void)upgrade_header;    /* Pre-fetched value, not used by IPPS */
    (void)connection_header; /* Pre-fetched value, not used by IPPS */

    /* Check for IPPS-specific headers */
    const char* printer_id = uvhttp_request_get_header(request, "X-Printer-ID");
    if (printer_id) {
        strncpy(protocol_name, "ipps", protocol_name_len);
        return 1;
    }
    return 0;
}

/**
 * @brief IPPS upgrade handler
 */
static uvhttp_error_t ipps_upgrade_handler(uvhttp_connection_t* conn,
                                            const char* protocol_name,
                                            void* user_data) {
    (void)protocol_name;  /* Unused parameter */
    
    ipps_context_t* ctx = (ipps_context_t*)user_data;
    
    /* 1. Send 101 Switching Protocols response */
    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, "Upgrade", "ipps");
    uvhttp_response_set_header(conn->response, "Connection", "Upgrade");
    uvhttp_response_send(conn->response);
    
    /* 2. Transfer connection ownership */
    return uvhttp_connection_transfer_ownership(conn, on_transfer_to_ipps, ctx);
}

/**
 * @brief Ownership transfer callback
 */
void on_transfer_to_ipps(uv_tcp_t* tcp_handle, int fd, void* user_data) {
    (void)tcp_handle;  // Unused parameter
    
    ipps_context_t* ctx = (ipps_context_t*)user_data;
    
    /* 1. Create IPPS connection */
    ipps_connection_t* ipps_conn = (ipps_connection_t*)uvhttp_alloc(sizeof(ipps_connection_t));
    if (!ipps_conn) {
        close(fd);
        return;
    }
    
    memset(ipps_conn, 0, sizeof(ipps_connection_t));
    ipps_conn->fd = fd;
    ipps_conn->ctx = ctx;
    
    /* 2. Get client IP */
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)&addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, ipps_conn->client_ip,
                      sizeof(ipps_conn->client_ip));
        }
    }
    
    /* 3. Set lifecycle callback */
    /* Note: In actual applications, lifecycle callback should be set to connection object
     * But since we have transferred ownership, connection object may have been released
     * So this is just an example, actual applications need more complex lifecycle management */
    
    /* 4. Start IPPS protocol processing */
    printf("IPPS connection established from %s, fd=%d\n",
           ipps_conn->client_ip, fd);
    
    ctx->connection_count++;
    
    /* Simulate IPPS protocol processing */
    /* In actual applications, IPPS protocol read and processing loop will be started here */
}

/**
 * @brief IPPS connection close callback
 */
void on_ipps_connection_close(void* user_data) {
    ipps_connection_t* ipps_conn = (ipps_connection_t*)user_data;
    
    if (ipps_conn) {
        if (ipps_conn->ctx) {
            ipps_conn->ctx->connection_count--;
        }
        
        close(ipps_conn->fd);
        uvhttp_free(ipps_conn);
    }
}

/* ========== Simulate gRPC-Web protocol ========== */

/**
 * @brief gRPC-Web context
 */
typedef struct {
    uv_loop_t* loop;
    int connection_count;
} grpc_context_t;

/**
 * @brief gRPC-Web connection
 */
typedef struct {
    int fd;
    char client_ip[INET6_ADDRSTRLEN];
    grpc_context_t* ctx;
} grpc_connection_t;

/**
 * @brief gRPC-Web protocol detector
 */
static int grpc_protocol_detector(uvhttp_request_t* request,
                                  char* protocol_name,
                                  size_t protocol_name_len,
                                  const char* upgrade_header,
                                  const char* connection_header) {
    (void)upgrade_header;    /* Pre-fetched value, not used by gRPC-Web */
    (void)connection_header; /* Pre-fetched value, not used by gRPC-Web */

    /* Check for gRPC-Web specific headers */
    const char* content_type = uvhttp_request_get_header(request, "Content-Type");
    if (content_type && strstr(content_type, "application/grpc-web") != NULL) {
        strncpy(protocol_name, "grpc-web", protocol_name_len);
        return 1;
    }
    return 0;
}

/**
 * @brief gRPC-Web upgrade handler
 */
static uvhttp_error_t grpc_upgrade_handler(uvhttp_connection_t* conn,
                                            const char* protocol_name,
                                            void* user_data) {
    (void)protocol_name;  /* Unused parameter */
    
    grpc_context_t* ctx = (grpc_context_t*)user_data;
    
    /* gRPC-Web does not send 101 response, upgrade directly */
    /* Transfer connection ownership */
    return uvhttp_connection_transfer_ownership(conn, on_transfer_to_grpc, ctx);
}

/**
 * @brief Ownership transfer callback
 */
void on_transfer_to_grpc(uv_tcp_t* tcp_handle, int fd, void* user_data) {
    (void)tcp_handle;  // Unused parameter
    
    grpc_context_t* ctx = (grpc_context_t*)user_data;
    
    /* 1. Create gRPC-Web connection */
    grpc_connection_t* grpc_conn = (grpc_connection_t*)uvhttp_alloc(sizeof(grpc_connection_t));
    if (!grpc_conn) {
        close(fd);
        return;
    }
    
    memset(grpc_conn, 0, sizeof(grpc_connection_t));
    grpc_conn->fd = fd;
    grpc_conn->ctx = ctx;
    
    /* 2. Get client IP */
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)&addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, grpc_conn->client_ip,
                      sizeof(grpc_conn->client_ip));
        }
    }
    
    /* 3. Start gRPC-Web protocol processing */
    printf("gRPC-Web connection established from %s, fd=%d\n",
           grpc_conn->client_ip, fd);
    
    ctx->connection_count++;
    
    /* Simulate gRPC-Web protocol processing */
    /* In actual applications, gRPC-Web protocol read and processing loop will be started here */
}

/* ========== HTTP request handler ========== */

/**
 * @brief HTTP request handler
 */
static int http_request_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Unused parameter */
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello from HTTP server", strlen("Hello from HTTP server"));
    uvhttp_response_send(response);
    
    return 0;
}

/* ========== Main function ========== */

int main(int argc, char* argv[]) {
    (void)argc;  /* Unused parameter */
    (void)argv;  /* Unused parameter */
    
    /* 1. Create event loop */
    uv_loop_t* loop = uv_default_loop();
    
    /* 2. Create IPPS context */
    ipps_context_t ipps_ctx;
    memset(&ipps_ctx, 0, sizeof(ipps_ctx));
    ipps_ctx.loop = loop;
    
    /* 3. Create gRPC-Web context */
    grpc_context_t grpc_ctx;
    memset(&grpc_ctx, 0, sizeof(grpc_ctx));
    grpc_ctx.loop = loop;
    
    /* 4. Create HTTP server */
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    /* 5. Register HTTP routes */
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_router_add_route(router, "/", http_request_handler);
    uvhttp_server_set_router(server, router);
    
    /* 6. Register IPPS protocol upgrade */
    result = uvhttp_server_register_protocol_upgrade(
        server,
        "ipps",
        "ipps",
        ipps_protocol_detector,
        ipps_upgrade_handler,
        &ipps_ctx
    );
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to register IPPS protocol: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 7. Register gRPC-Web protocol upgrade */
    result = uvhttp_server_register_protocol_upgrade(
        server,
        "grpc-web",
        "grpc-web",
        grpc_protocol_detector,
        grpc_upgrade_handler,
        &grpc_ctx
    );
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to register gRPC-Web protocol: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 8. Start server */
    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }
    
    printf("Server started on http://0.0.0.0:8080\n");
    printf("Try:\n");
    printf("  - HTTP: curl http://localhost:8080/\n");
    printf("  - IPPS: curl -H 'X-Printer-ID: printer1' http://localhost:8080/\n");
    printf("  - gRPC-Web: curl -H 'Content-Type: application/grpc-web' http://localhost:8080/\n");
    
    /* 9. Run event loop */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 10. Clean up resources */
    uvhttp_server_free(server);
    
    return 0;
}
