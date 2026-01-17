# API Documentation

## Overview

UVHTTP provides a concise C API for building high-performance HTTP/1.1 and WebSocket servers.

## Core Modules

### Server (uvhttp_server)

`uvhttp_server_t` is the core server structure.

#### Create Server

```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

#### Start Server

```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```

#### Stop Server

```c
void uvhttp_server_close(uvhttp_server_t* server);
```

### Router (uvhttp_router)

`uvhttp_router_t` provides routing functionality.

#### Create Router

```c
uvhttp_router_t* uvhttp_router_new(void);
```

#### Add Route

```c
void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_handler_t handler);
```

### Request (uvhttp_request)

`uvhttp_request_t` represents an HTTP request.

#### Get Request Method

```c
uvhttp_method_t uvhttp_request_get_method(uvhttp_request_t* req);
```

#### Get Request Path

```c
const char* uvhttp_request_get_path(uvhttp_request_t* req);
```

#### Get Request Header

```c
const char* uvhttp_request_get_header(uvhttp_request_t* req, const char* name);
```

#### Get Request Body

```c
const char* uvhttp_request_get_body(uvhttp_request_t* req, size_t* len);
```

### Response (uvhttp_response)

`uvhttp_response_t` is used to build HTTP responses.

#### Create Response

```c
uvhttp_response_t* uvhttp_response_new(uvhttp_request_t* req);
```

#### Set Status Code

```c
void uvhttp_response_set_status(uvhttp_response_t* res, int status);
```

#### Set Response Header

```c
void uvhttp_response_set_header(uvhttp_response_t* res, const char* name, const char* value);
```

#### Set Response Body

```c
void uvhttp_response_set_body(uvhttp_response_t* res, const char* body);
```

#### Send Response

```c
void uvhttp_response_send(uvhttp_response_t* res);
```

## Error Handling

All functions that may fail return `uvhttp_error_t`:

```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR = -1,
    UVHTTP_ERR_INVALID_PARAM = -2,
    UVHTTP_ERR_OUT_OF_MEMORY = -3,
    // ... more error codes
} uvhttp_error_t;
```

### Error Checking

```c
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}
```

## Complete Example

```c
#include <uvhttp.h>
#include <stdio.h>

void index_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html");
    uvhttp_response_set_body(res, "<h1>Hello, UVHTTP!</h1>");
    uvhttp_response_send(res);
}

void api_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "{\"message\":\"API response\"}");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/", index_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_close(server);
    return 0;
}
```