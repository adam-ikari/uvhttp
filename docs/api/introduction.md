# API Documentation

## Overview

UVHTTP provides a concise C API for building high-performance HTTP/1.1 and WebSocket servers.

## 📌 Platform Support

**Currently Supported**: Linux

**Planned**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for the Linux platform. We plan to expand support for other operating systems and platforms in future versions.

## Core Modules

### Server (uvhttp_server)

`uvhttp_server_t` is the core structure for the server.

#### Create Server

```c
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server);
```

#### Start Server

```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```

#### Stop Server

```c
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
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

`uvhttp_response_t` is used to build HTTP responses. The response object is created by the framework and passed to the request handler.

#### Set Status Code

```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status);
```

#### Set Response Header

```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
```

#### Set Response Body

```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
```

#### Send Response

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

## Error Handling

All functions that can fail return `uvhttp_error_t`:

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
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}
```

## Complete Example

```c
#include <uvhttp.h>
#include <stdio.h>
#include <string.h>

int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, "<h1>Hello, UVHTTP!</h1>", strlen("<h1>Hello, UVHTTP!</h1>"));
    uvhttp_response_send(response);
    return UVHTTP_OK;
}

int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    const char* json_body = "{\"message\":\"API response\"}";
    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);
    return UVHTTP_OK;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/", index_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_free(server);
    return 0;
}
```

## More APIs

The complete API documentation is being refined and currently includes the following core modules:

- **Server (uvhttp_server)** - Server creation, startup, and shutdown
- **Router (uvhttp_router)** - Route management and parameter extraction
- **Request (uvhttp_request)** - HTTP request handling
- **Response (uvhttp_response)** - HTTP response building
- **WebSocket (uvhttp_websocket)** - WebSocket connection management

Detailed API reference documentation is being written. Stay tuned!