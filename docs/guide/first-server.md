# Your First Server

Create your first UVHTTP server.

## Hello World Server

A simple HTTP server that responds with "Hello, World!".

### Code

Create `hello.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");

    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/", hello_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### Compile

```bash
# Method 1: Use CMake
make build
make hello_world

# Method 2: Compile directly
gcc -o hello hello.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

# Run
./hello_world
```

### Test

Visit `http://localhost:8080` in your browser, or:

```bash
curl http://localhost:8080/
```

## JSON API Server

An API server that returns JSON.

### Code

Create `json_api.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json; charset=utf-8");

    const char* json_body = "{"
        "\"status\": \"success\","
        "\"message\": \"Hello from UVHTTP!\","
        "\"version\": \"1.0.0\""
    "}";

    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);

    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/api", api_handler);
    uvhttp_router_add_route(router, "/api/status", api_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("JSON API server running at http://localhost:8080\n");
    printf("  - http://localhost:8080/api\n");
    printf("  - http://localhost:8080/api/status\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### Compile and Run

```bash
gcc -o json_api json_api.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./json_api
```

### Test

```bash
curl http://localhost:8080/api
```

Response:
```json
{
  "status": "success",
  "message": "Hello from UVHTTP!",
  "version": "1.0.0"
}
```

## Path Parameter Server

A server that handles path parameters.

### Code

Create `path_params.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int user_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    const char* username = path + 7;  // Skip "/users/"
    const char* format = uvhttp_request_get_query_param(request, "format");

    printf("Requested user: %s\n", username);

    uvhttp_response_set_status(response, 200);

    if (format && strcmp(format, "json") == 0) {
        char json_body[256];
        snprintf(json_body, sizeof(json_body),
                 "{\"username\":\"%s\",\"status\":\"active\"}", username);

        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, json_body, strlen(json_body));
    } else {
        char text_body[256];
        snprintf(text_body, sizeof(text_body),
                 "User: %s\nStatus: Active", username);

        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, text_body, strlen(text_body));
    }

    uvhttp_response_send(response);
    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/users/*", user_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("Path parameter server running at http://localhost:8080\n");
    printf("  - http://localhost:8080/users/alice\n");
    printf("  - http://localhost:8080/users/bob?format=json\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### Compile and Run

```bash
gcc -o path_params path_params.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./path_params
```

### Test

```bash
# Text format
curl http://localhost:8080/users/alice

# JSON format
curl http://localhost:8080/users/bob?format=json
```

## Next Steps

- Routing system - more complex routing configuration
- Request handling - HTTP request handling
- Response handling - sending different types of responses
- [Full tutorial](./TUTORIAL.md) - from basics to advanced

## FAQ

### Q: How do I listen on another port?

Change the port argument of `uvhttp_server_listen`:
```c
uvhttp_server_listen(g_server, "0.0.0.0", 9000);  // Use port 9000
```

### Q: How do I listen on a specific IP?

Change the host argument of `uvhttp_server_listen`:
```c
uvhttp_server_listen(g_server, "127.0.0.1", 8080);  // Listen on localhost only
```

### Q: How do I handle POST requests?

Check the request method in the handler:
```c
const char* method = uvhttp_request_get_method(request);
if (strcmp(method, "POST") == 0) {
    // Handle POST request
}
```

### Q: How do I get the request body?

Use `uvhttp_request_get_body`:
```c
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);
```
