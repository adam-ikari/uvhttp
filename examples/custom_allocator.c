#include "../include/uvhttp.h"
#include <stdio.h>

/* Custom allocator functions */
static void* custom_malloc(size_t size) {
    printf("Custom alloc: %zu bytes\n", size);
    return malloc(size);
}

static void custom_free(void* ptr) {
    printf("Custom free: %p\n", ptr);
    free(ptr);
}

static void* custom_realloc(void* ptr, size_t size) {
    printf("Custom realloc: %p -> %zu bytes\n", ptr, size);
    return realloc(ptr, size);
}

static void* custom_calloc(size_t nmemb, size_t size) {
    printf("Custom calloc: %zu * %zu bytes\n", nmemb, size);
    return calloc(nmemb, size);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Custom allocator example", 23);
    return UVHTTP_OK;
}

int main() {
    printf("Starting UVHTTP custom allocator example\n");
    
    /* Set custom allocator */
    static uvhttp_allocator_t custom_allocator = {
        .malloc = custom_malloc,
        .free = custom_free,
        .realloc = custom_realloc,
        .calloc = custom_calloc,
        .data = NULL,
        .type = UVHTTP_ALLOCATOR_CUSTOM
    };
    
    uvhttp_allocator_set(&custom_allocator);
    
    /* Create server */
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    
    /* Start server */
    uvhttp_error_t ret = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (ret != UVHTTP_OK) {
        fprintf(stderr, "Start failed: %s\n", uvhttp_error_string(ret));
        return 1;
    }
    
    printf("Server running at: http://localhost:8080\n");
    printf("Using custom memory allocator\n");
    
    uv_run(server->loop, UV_RUN_DEFAULT);
    
    /* Cleanup */
    uvhttp_server_free(server);
    
    return 0;
}