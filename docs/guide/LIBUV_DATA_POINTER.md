# Using the libuv Loop Data Pointer

## Overview

libuv's event loop provides a `data` pointer field that can store user-defined data. This is a best practice for avoiding global variables, especially in multithreaded environments.

## Basic Concepts

### The uv_loop_t data Field

```c
typedef struct uv_loop_s {
    // ... other fields ...
    void* data;  // user data pointer
    // ... other fields ...
} uv_loop_t;
```

### Why Use the data Pointer?

1. **Avoid global variables**: global variables cause race conditions in multithreaded environments
2. **Thread safety**: each event loop has its own data, isolated from one another
3. **Code encapsulation**: groups related data together, improving maintainability
4. **Context passing**: makes it easy to access application context from within callbacks

## Basic Usage

### 1. Define an Application Context Structure

```c
/**
 * @brief Application context structure
 *
 * Encapsulates all application-related data
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    time_t start_time;
    // other application data...
} app_context_t;
```

### 2. Create and Set the Context

```c
int main() {
    uv_loop_t* loop = uv_default_loop();

    // Create the application context
    app_context_t* ctx = malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize the context
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);

    // Create the server
    uvhttp_server_new(loop, &ctx->server);
    uvhttp_router_new(&ctx->router);
    uvhttp_server_set_router(ctx->server, ctx->router);

    // Set the context into the event loop's data pointer
    loop->data = ctx;

    // Add routes (needs access to the context)
    uvhttp_router_add_route(ctx->router, "/", home_handler);

    // Start the server
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);

    // Run the event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);

    return 0;
}
```

### 3. Access the Context Inside a Callback

```c
/**
 * @brief Request handler
 *
 * Retrieves the application context from the event loop's data pointer
 */
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Get the event loop
    uv_loop_t* loop = uv_default_loop();

    // Get the application context from the event loop
    app_context_t* ctx = (app_context_t*)loop->data;

    // Access context data
    ctx->request_count++;

    // Use data from the context
    char response[256];
    snprintf(response, sizeof(response),
        "{\"request_count\":%d,\"uptime\":%ld}",
        ctx->request_count,
        time(NULL) - ctx->start_time);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}
```

## Complete Examples

### Example 1: A Simple Counter

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

/**
 * @brief Application context
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    time_t start_time;
} app_context_t;

/**
 * @brief Stats handler
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;

    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"request_count\": %d,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"active_connections\": %zu\n"
        "}",
        ctx->request_count,
        time(NULL) - ctx->start_time,
        ctx->server ? ctx->server->active_connections : 0);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));

    ctx->request_count++;

    return uvhttp_response_send(res);
}

/**
 * @brief Signal handling
 */
void signal_handler(int sig) {
    printf("\nReceived signal %d\n", sig);
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    // Create and initialize the context
    app_context_t* ctx = malloc(sizeof(app_context_t));
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);

    // Set it into the event loop
    loop->data = ctx;

    // Create the server
    uvhttp_server_new(loop, &ctx->server);
    uvhttp_router_new(&ctx->router);
    uvhttp_server_set_router(ctx->server, ctx->router);

    // Add routes
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);

    // Start the server
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080/stats\n");

    // Run the event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);

    return 0;
}
```

### Example 2: Multithreaded Environment

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4

/**
 * @brief Worker thread context
 */
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} worker_context_t;

/**
 * @brief Request handler
 */
int worker_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    worker_context_t* ctx = (worker_context_t*)loop->data;

    char response[256];
    snprintf(response, sizeof(response),
        "{\"thread_id\":%d,\"request_count\":%d}",
        ctx->thread_id,
        ++ctx->request_count);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

/**
 * @brief Worker thread function
 */
void* worker_thread(void* arg) {
    worker_context_t* ctx = (worker_context_t*)arg;

    // Create a dedicated event loop
    ctx->loop = uv_loop_new();

    // Set the context into the event loop
    ctx->loop->data = ctx;

    // Create the server
    uvhttp_server_new(ctx->loop, &ctx->server);
    uvhttp_router_new(&ctx->router);
    uvhttp_server_set_router(ctx->server, ctx->router);

    // Add routes
    uvhttp_router_add_route(ctx->router, "/", worker_handler);

    // Start the server (on a different port)
    int port = 8080 + ctx->thread_id;
    uvhttp_server_listen(ctx->server, "0.0.0.0", port);

    printf("Thread %d listening on port %d\n", ctx->thread_id, port);

    // Run the event loop
    uv_run(ctx->loop, UV_RUN_DEFAULT);

    // Cleanup
    uvhttp_server_free(ctx->server);
    uv_loop_close(ctx->loop);
    free(ctx->loop);

    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    worker_context_t contexts[THREAD_COUNT];

    // Create worker threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        contexts[i].thread_id = i;
        contexts[i].request_count = 0;

        pthread_create(&threads[i], NULL, worker_thread, &contexts[i]);
    }

    printf("Multithreaded server started with %d threads\n", THREAD_COUNT);

    // Wait for all threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
```

## Advanced Usage

### 1. Nested Contexts

```c
typedef struct {
    app_context_t* app_ctx;
    int worker_id;
    int local_count;
} worker_context_t;

// In the main thread
app_context_t* app_ctx = malloc(sizeof(app_context_t));
app_ctx->server = server;
app_ctx->router = router;
loop->data = app_ctx;

// In a worker thread
worker_context_t* worker_ctx = malloc(sizeof(worker_context_t));
worker_ctx->app_ctx = app_ctx;  // shared application context
worker_ctx->worker_id = id;
worker_ctx->local_count = 0;
worker_loop->data = worker_ctx;
```

### 2. Context Initialization Function

```c
/**
 * @brief Initialize the application context
 */
app_context_t* app_context_create(uv_loop_t* loop) {
    app_context_t* ctx = malloc(sizeof(app_context_t));
    if (!ctx) return NULL;

    ctx->server = NULL;
    ctx->router = NULL;
    uvhttp_server_new(loop, &ctx->server);
    uvhttp_router_new(&ctx->router);
    ctx->request_count = 0;
    ctx->start_time = time(NULL);

    if (ctx->server && ctx->router) {
        uvhttp_server_set_router(ctx->server, ctx->router);
        loop->data = ctx;
        return ctx;
    }

    // Creation failed, clean up
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
    return NULL;
}

/**
 * @brief Destroy the application context
 */
void app_context_destroy(app_context_t* ctx) {
    if (!ctx) return;

    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
}
```

### 3. Use in Async Callbacks

```c
typedef struct {
    uv_work_t work_req;
    app_context_t* ctx;
    char query[256];
} async_query_t;

void async_work_cb(uv_work_t* req) {
    async_query_t* async = (async_query_t*)req->data;
    // Perform async work
    printf("Executing query: %s\n", async->query);
}

void async_after_cb(uv_work_t* req, int status) {
    async_query_t* async = (async_query_t*)req->data;
    app_context_t* ctx = async->ctx;

    // Access the context
    ctx->request_count++;

    free(async);
    free(req);
}

int async_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;

    // Create an async request
    async_query_t* async = malloc(sizeof(async_query_t));
    async->ctx = ctx;
    strcpy(async->query, "SELECT * FROM users");

    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = async;

    uv_queue_work(loop, work_req, async_work_cb, async_after_cb);

    return 0;
}
```

## Best Practices

### 1. Always Check the data Pointer

```c
int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;

    // Check whether the context exists
    if (!ctx) {
        const char* error = "{\"error\":\"context not initialized\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // Use the context
    // ...
}
```

### 2. Reset the data Pointer During Cleanup

```c
void cleanup(app_context_t* ctx, uv_loop_t* loop) {
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }

    // Reset the data pointer
    loop->data = NULL;

    free(ctx);
}
```

### 3. Use a Macro to Simplify Access

```c
#define GET_CTX(loop, ctx_type) ((ctx_type*)((loop)->data))

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop, app_context_t);

    if (!ctx) {
        // error handling
    }

    // Use ctx
}
```

### 4. Thread-Safe Context Access

```c
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    pthread_mutex_t mutex;
} app_context_t;

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop, app_context_t);

    // Lock to protect shared data
    pthread_mutex_lock(&ctx->mutex);
    ctx->request_count++;
    int count = ctx->request_count;
    pthread_mutex_unlock(&ctx->mutex);

    // Use the data
    // ...
}
```

## FAQ

### Q: How do I share data across multiple event loops?

A: Use a shared context structure; each event loop's data pointer points to the same context:

```c
app_context_t* shared_ctx = malloc(sizeof(app_context_t));

// Thread 1
loop1->data = shared_ctx;

// Thread 2
loop2->data = shared_ctx;

// Remember to protect shared data with a mutex
```

### Q: Will libuv modify the data pointer?

A: No. libuv never modifies the data pointer; it is entirely under user control.

### Q: Can I store arbitrary data in the data pointer?

A: Yes. `data` is of type `void*` and can store any pointer. Using a struct to organize related data is recommended.

### Q: How do I handle the case where the data pointer is NULL?

A: Always check whether the data pointer is NULL, especially in library functions or callbacks:

```c
void callback(uv_async_t* handle) {
    uv_loop_t* loop = handle->loop;
    app_context_t* ctx = (app_context_t*)loop->data;

    if (!ctx) {
        fprintf(stderr, "Error: context not initialized\n");
        return;
    }

    // Continue processing
}
```

## Summary

Using the libuv loop's data pointer is a best practice for avoiding global variables:

1. **Encapsulation**: encapsulates related data in a struct
2. **Thread safety**: each event loop has its own data copy
3. **Flexibility**: the context can be modified dynamically at runtime
4. **Maintainability**: clear code structure, easy to understand and maintain

In multithreaded and complex applications, always use the data pointer to manage application state!
