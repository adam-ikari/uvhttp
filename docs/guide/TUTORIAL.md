# UVHTTP Complete Tutorial

> This tutorial describes how to build an HTTP server with UVHTTP.

## Prerequisites

### Required Tools
- **GCC/Clang** - C compiler
- **CMake** - build system
- **Git** - version control (optional)

### Dependency Notes
UVHTTP uses a self-contained dependency management approach; all dependencies are included in the project source:
- **libuv** - asynchronous I/O library (at `deps/libuv/`)
- **llhttp** - HTTP parser (at `deps/llhttp/`)
- **mbedtls** - TLS/SSL support (at `deps/mbedtls/`)
- **cjson** - JSON handling (at `deps/cjson/`)
- **mimalloc** - memory allocator (at `deps/mimalloc/`)

**No additional system dependencies are required**; all dependencies are built automatically at compile time.

### Quick Start
```bash
# 1. Clone or enter the project directory
cd uvhttp

# 2. Build the project
make build

# 3. Build complete; the library is located in the build/ directory
```

See also: [Appendix: Dependency Management and Building](#appendix-dependency-management-and-building)

## Table of Contents

- [Part One: Getting Started](#part-one-getting-started)
  - [Chapter 1: Hello World - Your First HTTP Server](#chapter-1-hello-world---your-first-http-server)
  - [Chapter 2: Understanding Core Concepts](#chapter-2-understanding-core-concepts)
  - [Chapter 3: Routing System Basics](#chapter-3-routing-system-basics)
- [Part Two: Advanced Development](#part-two-advanced-development)
  - [Chapter 4: Complex Routing Configuration](#chapter-4-complex-routing-configuration)
  - [Chapter 5: Advanced Request Handling](#chapter-5-advanced-request-handling)
  - [Chapter 6: Response Handling Optimization](#chapter-6-response-handling-optimization)
- [Part Three: Advanced Architecture](#part-three-advanced-architecture)
  - [Chapter 7: Using the libuv Data Pointer](#chapter-7-using-the-libuv-data-pointer)
  - [Chapter 8: Multithreaded Server](#chapter-8-multithreaded-server)
  - [Chapter 9: Asynchronous Database Integration](#chapter-9-asynchronous-database-integration)
  - [Chapter 10: Load Balancing](#chapter-10-load-balancing)
- [Part Four: Production Practices](#part-four-production-practices)
  - [Chapter 11: Performance Optimization](#chapter-11-performance-optimization)
  - [Chapter 12: Security Configuration](#chapter-12-security-configuration)
  - [Chapter 13: Monitoring and Logging](#chapter-13-monitoring-and-logging)

---

## Part One: Getting Started

### Chapter 1: Hello World - Your First HTTP Server

#### 1.1 Environment Preparation

**Install build tools**:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git

# CentOS/RHEL
sudo yum install gcc gcc-c++ make cmake git

# macOS
xcode-select --install
brew install cmake git
```

**Get the source code**:
```bash
# Clone the repository (includes all dependencies)
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

> **Note**: the `--recurse-submodules` flag automatically clones all dependencies. If you forgot to use this flag, you can recover by running `git submodule update --init --recursive`.

# Or use an existing project
cd /path/to/uvhttp
```

**Build UVHTTP** (using the project's bundled dependencies):
```bash
make build
```

**Dependency notes**:
The UVHTTP project already includes the following dependencies; no additional installation is needed:
- **libuv** - located in the `deps/libuv/` directory
- **llhttp** - located in the `deps/llhttp/` directory
- **mbedtls** - located in the `deps/mbedtls/` directory
- **cjson** - located in the `deps/cjson/` directory
- **mimalloc** - located in the `deps/mimalloc/` directory
- **uthash** - located in the `deps/uthash/` directory
- **xxhash** - located in the `deps/xxhash/` directory

These dependencies are compiled and linked into the UVHTTP library automatically.

#### 1.2 The Simplest HTTP Server

Create `hello_world.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

// Request handler function
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set the response status code
    uvhttp_response_set_status(res, 200);

    // Set the response header
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");

    // Set the response body
    const char* body = "Hello, World!";
    uvhttp_response_set_body(res, body, strlen(body));

    // Send the response
    return uvhttp_response_send(res);
}

int main() {
    printf("Starting Hello World server...\n");

    // Create an event loop
    uv_loop_t* loop = uv_default_loop();

    // Create a server
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Server creation failed: %s\n", uvhttp_error_string(result));
        return 1;
    }

    // Create a router
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_set_router(server, router);

    // Add a route
    uvhttp_router_add_route(router, "/", hello_handler);

    // Start the server and listen
    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Server startup failed: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    printf("Press Ctrl+C to stop the server\n");

    // Run the event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // Clean up resources
    uvhttp_server_free(server);

    return 0;
}
```

**Build and run**:
```bash
# Method 1: compile with CMake (recommended)
# In the project root directory
mkdir -p examples

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(hello_world C)

set(CMAKE_C_STANDARD 11)

# Find UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../../include NO_DEFAULT_PATH)
find_library(UVHTTP_LIBRARY uvhttp PATHS ../.. NO_DEFAULT_PATH)

include_directories(${UVHTTP_INCLUDE_DIR})

add_executable(hello_world ../../examples/01_basics/01_hello_world.c)
target_link_libraries(hello_world ${UVHTTP_LIBRARY} uv pthread m)
EOF

# Build
make build

# Run
./hello_world
```

**Or use the project's unified build system**:
```bash
# In the project root directory
make build
make hello_world

# Run
./examples/hello_world
```

**Test**:
```bash
curl http://localhost:8080/
```

#### 1.3 Code Breakdown

**Core components**:
1. **Event loop (uv_loop_t)**: libuv's event loop, which handles all asynchronous operations
2. **Server (uvhttp_server_t)**: the HTTP server instance
3. **Router (uvhttp_router_t)**: route matching and dispatch
4. **Request handler**: the callback function that processes HTTP requests

**Workflow**:
```
client request → libuv receives → uvhttp parses → route matching → handler executes → response sent
```

---

### Chapter 2: Understanding Core Concepts

#### 2.1 UVHTTP Architecture

```
┌─────────────────────────────────────────┐
│        Application Layer (your code)    │
│    ┌──────────────┐    ┌──────────────┐ │
│    │ Request      │    │ Business     │ │
│    │ handlers     │    │ logic        │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│         API Layer (uvhttp)              │
│    ┌──────────────┐    ┌──────────────┐ │
│    │ Server API   │    │ Routing      │ │
│    │              │    │ system       │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│       Core Layer (uvhttp_core)          │
│    ┌──────────────┐    ┌──────────────┐ │
│    │ Request      │    │ Response     │ │
│    │ parsing      │    │ building     │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│      Base Layer (libuv, llhttp)         │
│    ┌──────────────┐    ┌──────────────┐ │
│    │ Event        │    │ HTTP         │ │
│    │ driven       │    │ parsing      │ │
│    └──────────────┘    └──────────────┘ │
└─────────────────────────────────────────┘
```

#### 2.2 Key Data Structures

**Request object (uvhttp_request_t)**:
```c
typedef struct uvhttp_request {
    uvhttp_method_t method;      // HTTP method (GET, POST, etc.)
    char url[2048];              // request URL
    uvhttp_header_t* headers;    // request header array
    size_t header_count;         // number of headers
    char* body;                  // request body
    size_t body_length;          // request body length
    // ... other fields
} uvhttp_request_t;
```

**Response object (uvhttp_response_t)**:
```c
typedef struct uvhttp_response {
    uv_tcp_t* client;            // client connection
    int status_code;             // HTTP status code
    uvhttp_header_t headers[64]; // response header array
    size_t header_count;         // number of headers
    char* body;                  // response body
    size_t body_length;          // response body length
    // ... other fields
} uvhttp_response_t;
```

#### 2.3 Event-Driven Model

**Single-threaded event loop**:
```c
// The event loop keeps running
uv_run(loop, UV_RUN_DEFAULT);

// Run modes
UV_RUN_DEFAULT  // run until there are no active handles
UV_RUN_ONCE     // run one iteration
UV_RUN_NOWAIT   // run once without blocking
```

**Asynchronous operations**:
```c
// All I/O operations are asynchronous
// They do not block the event loop
uv_write(&write_req, stream, &buf, 1, on_write_complete);
uv_read_start(stream, alloc_buffer, on_read_complete);
```

---

### Chapter 3: Routing System Basics

#### 3.1 Basic Routing

Create `examples/02_routing/01_simple_routing.c`:

```c
#include "uvhttp.h"
#include <stdio.h>

// Home page handler
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>Home</h1><p>Welcome to UVHTTP</p></body></html>";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));

    return uvhttp_response_send(res);
}

// About page handler
int about_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>About</h1><p>UVHTTP HTTP server</p></body></html>";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));

    return uvhttp_response_send(res);
}

// API handler
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"API response\",\"status\":\"ok\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add multiple routes
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/about", about_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Routes:\n");
    printf("  /        - home page\n");
    printf("  /about   - about page\n");
    printf("  /api     - API endpoint\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

**Build and run**:
```bash
# Build with CMake
make build

# Run
./examples/simple_routing

# Test
curl http://localhost:8080/
curl http://localhost:8080/about
curl http://localhost:8080/api
```

#### 3.2 Routing Parameters

Create `route_params.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// User detail handler
int user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Extract the user ID from the URL
    const char* url = uvhttp_request_get_url(req);

    // Simple path parsing (a real application should use routing parameters)
    char user_id[64] = {0};
    if (sscanf(url, "/user/%63s", user_id) == 1) {
        char response[512];
        snprintf(response, sizeof(response),
            "{\"user_id\":\"%s\",\"name\":\"User %s\",\"email\":\"user%s@example.com\"}",
            user_id, user_id, user_id);

        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, response, strlen(response));
    } else {
        const char* error = "{\"error\":\"invalid user ID\"}";
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
    }

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add a route with a parameter
    uvhttp_router_add_route(router, "/user/*", user_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test: curl http://localhost:8080/user/123\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

#### 3.3 HTTP Method Routing

Create `method_routing.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// GET request handler
int get_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"method\":\"GET\",\"message\":\"Get resource\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

// POST request handler
int post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);

    char response[512];
    if (body) {
        snprintf(response, sizeof(response),
            "{\"method\":\"POST\",\"message\":\"Create resource\",\"received\":\"%s\"}",
            body);
    } else {
        snprintf(response, sizeof(response),
            "{\"method\":\"POST\",\"message\":\"Create resource\",\"received\":null}");
    }

    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

// PUT request handler
int put_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);

    char response[512];
    if (body) {
        snprintf(response, sizeof(response),
            "{\"method\":\"PUT\",\"message\":\"Update resource\",\"received\":\"%s\"}",
            body);
    } else {
        snprintf(response, sizeof(response),
            "{\"method\":\"PUT\",\"message\":\"Update resource\",\"received\":null}");
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

// DELETE request handler
int delete_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"method\":\"DELETE\",\"message\":\"Delete resource\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add routes for different HTTP methods
    uvhttp_router_add_route(router, "/resource", get_handler);
    uvhttp_router_add_route(router, "/resource", post_handler);
    uvhttp_router_add_route(router, "/resource", put_handler);
    uvhttp_router_add_route(router, "/resource", delete_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test:\n");
    printf("  curl http://localhost:8080/resource\n");
    printf("  curl -X POST http://localhost:8080/resource -d '{\"name\":\"test\"}'\n");
    printf("  curl -X PUT http://localhost:8080/resource -d '{\"name\":\"updated\"}'\n");
    printf("  curl -X DELETE http://localhost:8080/resource\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

---

## Part Two: Advanced Development

### Chapter 4: Complex Routing Configuration

#### 4.1 Middleware Pattern

Create `auth.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// Authentication check function
int check_auth(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* auth = uvhttp_request_get_header(req, "Authorization");

    if (!auth || strcmp(auth, "Bearer secret-token") != 0) {
        const char* error = "{\"error\":\"unauthorized\",\"message\":\"Invalid auth token\"}";

        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_header(res, "WWW-Authenticate", "Bearer");
        uvhttp_response_set_body(res, error, strlen(error));

        return uvhttp_response_send(res);
    }

    // Authentication succeeded, continue processing
    return 0;
}

// Protected handler
int protected_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // First pass the auth check
    if (check_auth(req, res) != 0) {
        return 0; // auth failed, response already sent
    }

    const char* json = "{\"message\":\"Access granted\",\"data\":\"Sensitive info\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

// Public handler
int public_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"Public access\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add routes
    uvhttp_router_add_route(router, "/public", public_handler);
    uvhttp_router_add_route(router, "/protected", protected_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test:\n");
    printf("  curl http://localhost:8080/public\n");
    printf("  curl http://localhost:8080/protected\n");
    printf("  curl -H 'Authorization: Bearer secret-token' http://localhost:8080/protected\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

#### 4.2 Route Groups

Create `route_groups.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// API v1 route group
int api_v1_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v1\",\"resource\":\"users\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int api_v1_posts_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v1\",\"resource\":\"posts\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

// API v2 route group
int api_v2_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v2\",\"resource\":\"users\",\"features\":[\"pagination\",\"filtering\"]}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int api_v2_posts_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v2\",\"resource\":\"posts\",\"features\":[\"pagination\",\"filtering\",\"sorting\"]}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // API v1 route group
    uvhttp_router_add_route(router, "/api/v1/users", api_v1_users_handler);
    uvhttp_router_add_route(router, "/api/v1/posts", api_v1_posts_handler);

    // API v2 route group
    uvhttp_router_add_route(router, "/api/v2/users", api_v2_users_handler);
    uvhttp_router_add_route(router, "/api/v2/posts", api_v2_posts_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("API routes:\n");
    printf("  /api/v1/users  - user list (v1)\n");
    printf("  /api/v1/posts  - post list (v1)\n");
    printf("  /api/v2/users  - user list (v2)\n");
    printf("  /api/v2/posts  - post list (v2)\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

---

### Chapter 5: Advanced Request Handling

#### 5.1 Request Header Handling

Create `request_headers.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// Request header info handler
int headers_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[4096];
    int pos = 0;

    // Build a JSON response
    pos += snprintf(response + pos, sizeof(response) - pos, "{\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"method\": \"%s\",\n",
                   uvhttp_request_get_method(req));
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"url\": \"%s\",\n",
                   uvhttp_request_get_url(req));
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"headers\": {\n");

    // Get common request headers
    const char* user_agent = uvhttp_request_get_header(req, "User-Agent");
    const char* accept = uvhttp_request_get_header(req, "Accept");
    const char* content_type = uvhttp_request_get_header(req, "Content-Type");
    const char* authorization = uvhttp_request_get_header(req, "Authorization");

    if (user_agent) {
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    \"User-Agent\": \"%s\",\n", user_agent);
    }
    if (accept) {
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    \"Accept\": \"%s\",\n", accept);
    }
    if (content_type) {
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    \"Content-Type\": \"%s\",\n", content_type);
    }
    if (authorization) {
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    \"Authorization\": \"***\"\n");
    }

    // Remove the trailing comma
    if (pos > 0 && response[pos - 2] == ',') {
        pos -= 2;
    }

    pos += snprintf(response + pos, sizeof(response) - pos, "  }\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "}\n");

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/headers", headers_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test: curl -v http://localhost:8080/headers\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

#### 5.2 Request Body Handling

Create `request_body.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// JSON POST handler
int json_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);

    if (!body || strlen(body) == 0) {
        const char* error = "{\"error\":\"Empty request body\"}";

        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));

        return uvhttp_response_send(res);
    }

    // Validate Content-Type
    const char* content_type = uvhttp_request_get_header(req, "Content-Type");
    if (!content_type || strstr(content_type, "application/json") == NULL) {
        const char* error = "{\"error\":\"Unsupported Content-Type\",\"expected\":\"application/json\"}";

        uvhttp_response_set_status(res, 415);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));

        return uvhttp_response_send(res);
    }

    // Process the JSON data (simple echo here)
    char response[4096];
    snprintf(response, sizeof(response),
        "{\"status\":\"success\",\"received\":\"%s\",\"length\":%zu}",
        body, strlen(body));

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

// File upload handler
int upload_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    size_t body_length = 0;

    if (body) {
        body_length = strlen(body);
    }

    char response[512];
    snprintf(response, sizeof(response),
        "{\"status\":\"received\",\"filename\":\"uploaded.dat\",\"size\":%zu}",
        body_length);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/api/json", json_post_handler);
    uvhttp_router_add_route(router, "/api/upload", upload_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test:\n");
    printf("  curl -X POST http://localhost:8080/api/json \\\n");
    printf("       -H 'Content-Type: application/json' \\\n");
    printf("       -d '{\"name\":\"test\"}'\n");
    printf("  curl -X POST http://localhost:8080/api/upload \\\n");
    printf("       -F 'file=@/path/to/file'\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

---

### Chapter 6: Response Handling Optimization

#### 6.1 Static File Middleware

Create `static_files.c`:

```c
#include "uvhttp.h"
#include "uvhttp_static.h"
#include <stdio.h>
#include <string.h>

// Static file serving context
static uvhttp_static_context_t* g_static_ctx = NULL;

/**
 * @brief Static file request handler
 */
int static_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    if (!g_static_ctx) {
        const char* error = "{\"error\":\"Static file service not initialized\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // Handle the static file request
    int result = uvhttp_static_handle_request(g_static_ctx, req, res);
    if (result != 0) {
        const char* error = "{\"error\":\"File not found\"}";
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    return 0;
}

/**
 * @brief Home page handler
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP Static File Server</title>"
        "<meta charset='utf-8'>"
        "</head>"
        "<body>"
        "<h1>🚀 UVHTTP Static File Server</h1>"
        "<p>Access the following files:</p>"
        "<ul>"
        "<li><a href='/index.html'>index.html</a></li>"
        "<li><a href='/about.html'>about.html</a></li>"
        "<li><a href='/style.css'>style.css</a></li>"
        "</ul>"
        "</body>"
        "</html>";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));

    return uvhttp_response_send(res);
}

int main() {
    printf("Starting static file server...\n");

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Configure the static file service
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB cache
        .cache_ttl = 3600,                   // 1 hour TTL
        .custom_headers = ""
    };

    // Create the static file service context
    uvhttp_static_create(&static_config, &g_static_ctx);
    if (!g_static_ctx) {
        fprintf(stderr, "Error: Failed to create static file service context\n");
        return 1;
    }

    printf("✓ Static file service configured\n");
    printf("  Root directory: %s\n", static_config.root_directory);
    printf("  Index file: %s\n", static_config.index_file);

    // Add routes
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/static/*", static_file_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("\n========================================\n");
    printf("  Server running at http://localhost:8080\n");
    printf("========================================\n\n");

    printf("Test:\n");
    printf("  curl http://localhost:8080/\n");
    printf("  curl http://localhost:8080/static/index.html\n");
    printf("  curl http://localhost:8080/static/about.html\n\n");

    printf("Press Ctrl+C to stop the server\n\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
    }
    uvhttp_server_free(server);

    return 0;
}
```

**Create test files**:
```bash
# Create the public directory
mkdir -p public

# Create index.html
cat > public/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>UVHTTP Static File Server</title>
    <link rel="stylesheet" href="/static/style.css">
</head>
<body>
    <h1>Welcome to UVHTTP</h1>
    <p>This is a static file server example.</p>
    <a href="/static/about.html">About us</a>
</body>
</html>
EOF

# Create about.html
cat > public/about.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>About us</title>
    <link rel="stylesheet" href="/static/style.css">
</head>
<body>
    <h1>About UVHTTP</h1>
    <p>UVHTTP is an HTTP server library.</p>
    <a href="/static/index.html">Back to home</a>
</body>
</html>
EOF

# Create style.css
cat > public/style.css << 'EOF'
body {
    font-family: Arial, sans-serif;
    margin: 40px;
    background: #f5f5f5;
}
h1 {
    color: #007bff;
}
a {
    color: #007bff;
    text-decoration: none;
}
a:hover {
    text-decoration: underline;
}
EOF
```

**Build and run**:
```bash
make build
./examples/static_files

# Test
curl http://localhost:8080/
curl http://localhost:8080/static/index.html
```

#### 6.2 Unified Response Handling

Create `unified_response.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// JSON response helper function
void send_json_response(uvhttp_response_t* res, int status, const char* json_data) {
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_data, strlen(json_data));
    uvhttp_response_send(res);
}

// HTML response helper function
void send_html_response(uvhttp_response_t* res, int status, const char* html_data) {
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html_data, strlen(html_data));
    uvhttp_response_send(res);
}

// Error response helper function
void send_error_response(uvhttp_response_t* res, int status, const char* error, const char* message) {
    char response[512];
    snprintf(response, sizeof(response),
        "{\"error\":\"%s\",\"message\":\"%s\",\"status\":%d}",
        error, message, status);

    send_json_response(res, status, response);
}

// Handler using unified responses
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);

    if (strcmp(method, "GET") == 0) {
        // GET request - return data
        const char* json = "{\"data\":[{\"id\":1,\"name\":\"Item 1\"},{\"id\":2,\"name\":\"Item 2\"}]}";
        send_json_response(res, 200, json);
    } else if (strcmp(method, "POST") == 0) {
        // POST request - create a resource
        const char* body = uvhttp_request_get_body(req);
        if (!body) {
            send_error_response(res, 400, "missing_body", "Request body missing");
        } else {
            const char* json = "{\"status\":\"created\",\"id\":123}";
            send_json_response(res, 201, json);
        }
    } else {
        // Unsupported method
        send_error_response(res, 405, "method_not_allowed", "Unsupported HTTP method");
    }

    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/api", api_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test:\n");
    printf("  curl http://localhost:8080/api\n");
    printf("  curl -X POST http://localhost:8080/api -d '{\"name\":\"test\"}'\n");
    printf("  curl -X PUT http://localhost:8080/api -d '{\"name\":\"test\"}'\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

**For more details**, refer to the [Unified Response Guide](UNIFIED_RESPONSE_GUIDE.md).

#### 6.3 Streaming Responses

Create `streaming_response.c`:

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// Streaming data handler
int stream_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set streaming response headers
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_header(res, "Transfer-Encoding", "chunked");
    uvhttp_response_set_header(res, "Cache-Control", "no-cache");

    // Send the initial response headers
    uvhttp_response_send(res);

    // Note: real streaming responses require a more complex implementation
    // This only demonstrates the concept

    // In a real application, you can:
    // 1. Use libuv asynchronous writes
    // 2. Send data in batches
    // 3. Keep the connection open and keep sending data

    const char* message = "Streaming response data\n";
    uvhttp_response_set_body(res, message, strlen(message));

    return 0;
}

// Server-Sent Events (SSE) handler
int sse_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set SSE response headers
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/event-stream");
    uvhttp_response_set_header(res, "Cache-Control", "no-cache");
    uvhttp_response_set_header(res, "Connection", "keep-alive");

    // Send the response headers
    uvhttp_response_send(res);

    // Note: real SSE requires continuously sending events
    // This only demonstrates the concept

    const char* event = "event: message\ndata: Hello from SSE\n\n";
    uvhttp_response_set_body(res, event, strlen(event));

    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/stream", stream_handler);
    uvhttp_router_add_route(router, "/sse", sse_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Test:\n");
    printf("  curl http://localhost:8080/stream\n");
    printf("  curl -N http://localhost:8080/sse\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}
```

---

## Part Three: Advanced Architecture

### Chapter 7: Using the libuv Data Pointer

#### 7.1 Why Do You Need the Data Pointer

When developing an HTTP server, we often need to store application state, such as:
- Server configuration
- Request counters
- Database connection pools
- Cache objects

**Problems with traditional methods**:
```c
// ❌ Global variables - not thread-safe
static uvhttp_server_t* g_server = NULL;
static int g_request_count = 0;
```

**A better approach**:
```c
// ✅ libuv data pointer - thread-safe
typedef struct {
    uvhttp_server_t* server;
    int request_count;
    // other application data...
} app_context_t;

// Store the context in the event loop
loop->data = ctx;
```

#### 7.2 Creating an Application Context

```c
#include "uvhttp.h"
#include <time.h>

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
    char server_name[64];
} app_context_t;

/**
 * @brief Create an application context
 */
app_context_t* app_context_create(uv_loop_t* loop, const char* name) {
    // Allocate memory
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        return NULL;
    }

    // Initialize
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    strncpy(ctx->server_name, name, sizeof(ctx->server_name) - 1);

    // Create the server
    uvhttp_server_new(loop, &ctx->server);
    if (!ctx->server) {
        free(ctx);
        return NULL;
    }

    // Create the router
    uvhttp_router_new(&ctx->router);
    if (!ctx->router) {
        uvhttp_server_free(ctx->server);
        free(ctx);
        return NULL;
    }

    // Set the router
    uvhttp_server_set_router(ctx->server, ctx->router);

    // Set the context into the event loop
    loop->data = ctx;

    return ctx;
}

/**
 * @brief Destroy an application context
 */
void app_context_destroy(app_context_t* ctx, uv_loop_t* loop) {
    if (!ctx) return;

    // Clean up the server
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }

    // Reset the data pointer
    loop->data = NULL;

    free(ctx);
}
```

#### 7.3 Accessing the Context in a Handler

```c
// Convenience macro
#define GET_CTX(loop) ((app_context_t*)((loop)->data))

/**
 * @brief Stats handler
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Get the event loop
    uv_loop_t* loop = uv_default_loop();

    // Get the application context
    app_context_t* ctx = GET_CTX(loop);

    // Check whether the context exists
    if (!ctx) {
        const char* error = "{\"error\":\"context not initialized\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // Use the context data
    ctx->request_count++;

    long uptime = time(NULL) - ctx->start_time;

    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"server_name\": \"%s\",\n"
        "  \"request_count\": %d,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"active_connections\": %zu\n"
        "}",
        ctx->server_name,
        ctx->request_count,
        uptime,
        ctx->server ? ctx->server->active_connections : 0);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}
```

#### 7.4 Complete Example

```c
int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    // Create the application context
    app_context_t* ctx = app_context_create(loop, "MyServer");
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create application context\n");
        return 1;
    }

    // Add routes
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);

    // Start the server
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");

    // Run the event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    app_context_destroy(ctx, loop);

    return 0;
}
```

#### 7.5 Use in a Multithreaded Environment

```c
// Worker thread context
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    app_context_t* app_ctx;  // shared application context
    pthread_mutex_t mutex;
} worker_context_t;

void* worker_thread(void* arg) {
    worker_context_t* worker = (worker_context_t*)arg;

    // Create a dedicated event loop
    worker->loop = uv_loop_new();

    // Create a thread-specific context
    app_context_t* thread_ctx = malloc(sizeof(app_context_t));
    thread_ctx->server = NULL;
    thread_ctx->router = NULL;
    uvhttp_server_new(worker->loop, &thread_ctx->server);
    uvhttp_router_new(&thread_ctx->router);

    // Set it into the event loop
    worker->loop->data = thread_ctx;

    // Run the event loop
    uv_run(worker->loop, UV_RUN_DEFAULT);

    return NULL;
}
```

**Detailed tutorial**: see the [Complete Guide to the libuv Data Pointer](LIBUV_DATA_POINTER.md)

### Chapter 8: Multithreaded Server

#### 8.1 Understanding the Multithreaded Architecture

**Single-threaded vs multithreaded**:

```
Single-threaded model:
┌─────────────────┐
│  Event Loop     │
│  (main thread)  │
└─────────────────┘
    ↓
┌─────────────────┐
│  All requests   │
│  processed      │
│  serially       │
└─────────────────┘

Multithreaded model:
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  Event Loop     │  │  Event Loop     │  │  Event Loop     │
│  (thread 1)     │  │  (thread 2)     │  │  (thread 3)     │
└─────────────────┘  └─────────────────┘  └─────────────────┘
       ↓                    ↓                    ↓
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  Some requests  │  │  Some requests  │  │  Some requests  │
│  processed in   │  │  processed in   │  │  processed in   │
│  parallel       │  │  parallel       │  │  parallel       │
└─────────────────┘  └─────────────────┘  └─────────────────┘
```

#### 8.2 Multithreaded Server Implementation

Create `multithreaded_server.c`:

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4
#define PORT_BASE 8080

// Thread data structure
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uv_async_t async;
    int is_running;
} worker_thread_t;

static worker_thread_t workers[THREAD_COUNT];

// Request handler
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[256];
    snprintf(response, sizeof(response),
        "{\"message\":\"Hello from thread\",\"thread_id\":%d}",
        pthread_self() % 1000);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

// Worker thread function
void* worker_thread_func(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;

    printf("Worker thread %d started\n", worker->thread_id);

    // Create an event loop
    worker->loop = uv_loop_new();

    // Create a server
    worker->server = NULL;
    uvhttp_server_new(worker->loop, &worker->server);
    if (!worker->server) {
        fprintf(stderr, "Thread %d: server creation failed\n", worker->thread_id);
        return NULL;
    }

    // Create a router
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_router_add_route(router, "/", request_handler);
    uvhttp_server_set_router(worker->server, router);

    // Start listening (each thread listens on a different port)
    int port = PORT_BASE + worker->thread_id;
    int result = uvhttp_server_listen(worker->server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Thread %d: server startup failed: %d\n", worker->thread_id, result);
        return NULL;
    }

    printf("Thread %d: server listening on port %d\n", worker->thread_id, port);

    // Run the event loop
    worker->is_running = 1;
    uv_run(worker->loop, UV_RUN_DEFAULT);

    printf("Worker thread %d exiting\n", worker->thread_id);

    return NULL;
}

// Graceful shutdown
void shutdown_handler(uv_async_t* async) {
    worker_thread_t* worker = (worker_thread_t*)async->data;

    printf("Shutting down thread %d\n", worker->thread_id);

    // Stop the server
    if (worker->server) {
        uvhttp_server_stop(worker->server);
    }

    // Stop the event loop
    if (worker->loop) {
        uv_stop(worker->loop);
    }

    worker->is_running = 0;
}

int main() {
    pthread_t threads[THREAD_COUNT];

    printf("Starting multithreaded HTTP server\n");
    printf("Thread count: %d\n", THREAD_COUNT);
    printf("Port range: %d-%d\n", PORT_BASE, PORT_BASE + THREAD_COUNT - 1);

    // Create worker threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        workers[i].thread_id = i;
        workers[i].is_running = 0;

        // Initialize the async handle
        uv_async_init(uv_default_loop(), &workers[i].async, shutdown_handler);
        workers[i].async.data = &workers[i];

        // Create the thread
        int result = pthread_create(&threads[i], NULL, worker_thread_func, &workers[i]);
        if (result != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    printf("All threads started\n");
    printf("Press Ctrl+C to stop the server\n");

    // Wait for a signal
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    // Send shutdown signals to all worker threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        uv_async_send(&workers[i].async);
    }

    // Wait for all threads to finish
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);

        // Clean up resources
        if (workers[i].server) {
            uvhttp_server_free(workers[i].server);
        }
        if (workers[i].loop) {
            uv_loop_close(workers[i].loop);
            free(workers[i].loop);
        }
    }

    printf("Server shut down\n");

    return 0;
}
```

**Build and run**:
```bash
gcc -o multithreaded_server multithreaded_server.c \
    -I../include \
    -L../build \
    -luvhttp -luv -lpthread

./multithreaded_server
```

**Test**:
```bash
# Test different threads
curl http://localhost:8080/
curl http://localhost:8081/
curl http://localhost:8082/
curl http://localhost:8083/
```

---

### Chapter 9: Asynchronous Database Integration

#### 9.1 Asynchronous Database Connections

Create `async_database.c`:

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Database connection structure
typedef struct {
    char host[128];
    int port;
    char database[64];
    char username[64];
    char password[64];
    int is_connected;
    pthread_mutex_t mutex;
} database_connection_t;

// Database query request
typedef struct {
    uvhttp_request_t* request;
    uvhttp_response_t* response;
    char query[512];
    database_connection_t* db;
} db_query_request_t;

static database_connection_t g_db = {0};

// Initialize the database connection
int db_init(database_connection_t* db) {
    strcpy(db->host, "localhost");
    db->port = 3306;
    strcpy(db->database, "testdb");
    strcpy(db->username, "root");
    strcpy(db->password, "password");
    db->is_connected = 0;
    pthread_mutex_init(&db->mutex, NULL);

    // In a real application, a real database connection would be established here
    printf("Database connection initialized: %s@%s:%d/%s\n",
           db->username, db->host, db->port, db->database);

    return 0;
}

// Asynchronous database query callback
void on_db_query_complete(uv_work_t* req, int status) {
    db_query_request_t* query_req = (db_query_request_t*)req->data;

    // Simulate the query result
    char result[1024];
    snprintf(result, sizeof(result),
        "{\"status\":\"success\",\"query\":\"%s\",\"data\":[{\"id\":1,\"name\":\"Item 1\"},{\"id\":2,\"name\":\"Item 2\"}]}",
        query_req->query);

    // Send the response
    uvhttp_response_set_status(query_req->response, 200);
    uvhttp_response_set_header(query_req->response, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(query_req->response, result, strlen(result));
    uvhttp_response_send(query_req->response);

    // Cleanup
    free(query_req);
    free(req);
}

// Database query work function
void db_query_work(uv_work_t* req) {
    db_query_request_t* query_req = (db_query_request_t*)req->data;

    // Simulate a database query (a real application should execute a real database operation)
    printf("Executing query: %s\n", query_req->query);

    // Lock to protect the database connection
    pthread_mutex_lock(&query_req->db->mutex);

    // Simulate query latency
    usleep(10000); // 10ms

    pthread_mutex_unlock(&query_req->db->mutex);
}

// API handler - get user list
int get_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Create an async query request
    db_query_request_t* query_req = malloc(sizeof(db_query_request_t));
    if (!query_req) {
        const char* error = "{\"error\":\"Memory allocation failed\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }

    query_req->request = req;
    query_req->response = res;
    strcpy(query_req->query, "SELECT * FROM users");
    query_req->db = &g_db;

    // Create a work request
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = query_req;

    // Execute the database query in the thread pool
    uv_queue_work(uv_default_loop(), work_req, db_query_work, on_db_query_complete);

    return 0; // handled asynchronously, response is not sent immediately
}

// API handler - create a user
int create_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);

    if (!body) {
        const char* error = "{\"error\":\"Empty request body\"}";
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }

    // Create an async query request
    db_query_request_t* query_req = malloc(sizeof(db_query_request_t));
    if (!query_req) {
        const char* error = "{\"error\":\"Memory allocation failed\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }

    query_req->request = req;
    query_req->response = res;
    snprintf(query_req->query, sizeof(query_req->query),
             "INSERT INTO users VALUES (%s)", body);
    query_req->db = &g_db;

    // Create a work request
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = query_req;

    // Execute the database query in the thread pool
    uv_queue_work(uv_default_loop(), work_req, db_query_work, on_db_query_complete);

    return 0;
}

int main() {
    // Initialize the database
    db_init(&g_db);

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add API routes
    uvhttp_router_add_route(router, "/api/users", get_users_handler);
    uvhttp_router_add_route(router, "/api/users", create_user_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Asynchronous database integration demo\n");
    printf("Test:\n");
    printf("  curl http://localhost:8080/api/users\n");
    printf("  curl -X POST http://localhost:8080/api/users -d '{\"name\":\"test\"}'\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    pthread_mutex_destroy(&g_db.mutex);
    uvhttp_server_free(server);

    return 0;
}
```

#### 9.2 Connection Pool Management

Create `connection_pool.c`:

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CONNECTIONS 10

// Database connection structure
typedef struct {
    int id;
    int is_used;
    time_t last_used;
    pthread_mutex_t mutex;
} db_connection_t;

// Connection pool structure
typedef struct {
    db_connection_t connections[MAX_CONNECTIONS];
    int total_connections;
    pthread_mutex_t pool_mutex;
} connection_pool_t;

static connection_pool_t g_pool = {0};

// Initialize the connection pool
void connection_pool_init(connection_pool_t* pool, int size) {
    pool->total_connections = size;
    pthread_mutex_init(&pool->pool_mutex, NULL);

    for (int i = 0; i < size; i++) {
        pool->connections[i].id = i;
        pool->connections[i].is_used = 0;
        pool->connections[i].last_used = 0;
        pthread_mutex_init(&pool->connections[i].mutex, NULL);
    }

    printf("Connection pool initialized, maximum connections: %d\n", size);
}

// Acquire a connection
db_connection_t* connection_pool_acquire(connection_pool_t* pool) {
    pthread_mutex_lock(&pool->pool_mutex);

    db_connection_t* conn = NULL;

    // Find an available connection
    for (int i = 0; i < pool->total_connections; i++) {
        if (!pool->connections[i].is_used) {
            pool->connections[i].is_used = 1;
            pool->connections[i].last_used = time(NULL);
            conn = &pool->connections[i];
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_mutex);

    if (conn) {
        printf("Acquired connection %d\n", conn->id);
    } else {
        printf("Warning: no available connection\n");
    }

    return conn;
}

// Release a connection
void connection_pool_release(connection_pool_t* pool, db_connection_t* conn) {
    if (!conn) return;

    pthread_mutex_lock(&pool->pool_mutex);

    conn->is_used = 0;
    conn->last_used = time(NULL);

    printf("Released connection %d\n", conn->id);

    pthread_mutex_unlock(&pool->pool_mutex);
}

// API handler - use the connection pool
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Acquire a connection from the pool
    db_connection_t* conn = connection_pool_acquire(&g_pool);

    if (!conn) {
        const char* error = "{\"error\":\"No available database connection\"}";
        uvhttp_response_set_status(res, 503);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }

    // Use the connection to execute a query (simulated)
    pthread_mutex_lock(&conn->mutex);
    printf("Executing query with connection %d\n", conn->id);
    usleep(5000); // simulate query latency
    pthread_mutex_unlock(&conn->mutex);

    // Send the response
    const char* json = "{\"status\":\"success\",\"connection_id\":1}";
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    uvhttp_response_send(res);

    // Release the connection
    connection_pool_release(&g_pool, conn);

    return 0;
}

int main() {
    // Initialize the connection pool
    connection_pool_init(&g_pool, MAX_CONNECTIONS);

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/api", api_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    printf("Connection pool management demo\n");
    printf("Test: curl http://localhost:8080/api\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    pthread_mutex_destroy(&g_pool.pool_mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        pthread_mutex_destroy(&g_pool.connections[i].mutex);
    }

    uvhttp_server_free(server);

    return 0;
}
```

---

### Chapter 10: Load Balancing

#### 10.1 Understanding Load Balancing

**Load balancing strategies**:

```
client request
    ↓
load balancer
    ↓
┌──────────┬──────────┬──────────┐
│ Server 1 │ Server 2 │ Server 3 │
│ (thread1)│ (thread2)│ (thread3)│
└──────────┴──────────┴──────────┘
```

#### 10.2 A Simple Load Balancer

Create `load_balancer.c`:

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BACKEND_COUNT 3
#define BACKEND_PORTS {8081, 8082, 8083}

// Backend server info
typedef struct {
    char host[128];
    int port;
    int is_healthy;
    int request_count;
    pthread_mutex_t mutex;
} backend_server_t;

static backend_server_t backends[BACKEND_COUNT];

// Initialize the backend servers
void init_backends() {
    int ports[] = BACKEND_PORTS;

    for (int i = 0; i < BACKEND_COUNT; i++) {
        strcpy(backends[i].host, "localhost");
        backends[i].port = ports[i];
        backends[i].is_healthy = 1;
        backends[i].request_count = 0;
        pthread_mutex_init(&backends[i].mutex, NULL);

        printf("Backend server %d: %s:%d\n", i, backends[i].host, backends[i].port);
    }
}

// Round-robin backend selection
backend_server_t* select_backend_round_robin() {
    static int current = 0;

    for (int i = 0; i < BACKEND_COUNT; i++) {
        int index = (current + i) % BACKEND_COUNT;
        if (backends[index].is_healthy) {
            current = (index + 1) % BACKEND_COUNT;
            return &backends[index];
        }
    }

    return NULL;
}

// Least-connections backend selection
backend_server_t* select_backend_least_connections() {
    backend_server_t* selected = NULL;
    int min_connections = -1;

    for (int i = 0; i < BACKEND_COUNT; i++) {
        if (backends[i].is_healthy) {
            pthread_mutex_lock(&backends[i].mutex);
            int connections = backends[i].request_count;
            pthread_mutex_unlock(&backends[i].mutex);

            if (min_connections == -1 || connections < min_connections) {
                min_connections = connections;
                selected = &backends[i];
            }
        }
    }

    return selected;
}

// Load balance handler
int load_balance_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Select a backend server (using round-robin)
    backend_server_t* backend = select_backend_round_robin();

    if (!backend) {
        const char* error = "{\"error\":\"No available backend server\"}";
        uvhttp_response_set_status(res, 503);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }

    // Increment the request count
    pthread_mutex_lock(&backend->mutex);
    backend->request_count++;
    pthread_mutex_unlock(&backend->mutex);

    // Simulate forwarding the request to the backend
    printf("Forwarding request to backend: %s:%d\n", backend->host, backend->port);

    // Send the response
    char response[512];
    snprintf(response, sizeof(response),
        "{\"status\":\"forwarded\",\"backend\":\"%s:%d\",\"request_count\":%d}",
        backend->host, backend->port, backend->request_count);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    uvhttp_response_send(res);

    // Decrement the request count
    pthread_mutex_lock(&backend->mutex);
    backend->request_count--;
    pthread_mutex_unlock(&backend->mutex);

    return 0;
}

// Health check handler
int health_check_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[1024];
    int pos = 0;

    pos += snprintf(response + pos, sizeof(response) - pos, "{\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"backends\": [\n");

    for (int i = 0; i < BACKEND_COUNT; i++) {
        pthread_mutex_lock(&backends[i].mutex);
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    {\"id\":%d,\"host\":\"%s\",\"port\":%d,\"healthy\":%s,\"connections\":%d}%s\n",
                       i, backends[i].host, backends[i].port,
                       backends[i].is_healthy ? "true" : "false",
                       backends[i].request_count,
                       i < BACKEND_COUNT - 1 ? "," : "");
        pthread_mutex_unlock(&backends[i].mutex);
    }

    pos += snprintf(response + pos, sizeof(response) - pos, "  ]\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "}\n");

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    uvhttp_response_send(res);

    return 0;
}

int main() {
    // Initialize the backend servers
    init_backends();

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // Add routes
    uvhttp_router_add_route(router, "/", load_balance_handler);
    uvhttp_router_add_route(router, "/health", health_check_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Load balancer running at http://localhost:8080\n");
    printf("Backend servers:\n");
    for (int i = 0; i < BACKEND_COUNT; i++) {
        printf("  %d. %s:%d\n", i, backends[i].host, backends[i].port);
    }
    printf("Test:\n");
    printf("  curl http://localhost:8080/\n");
    printf("  curl http://localhost:8080/health\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    for (int i = 0; i < BACKEND_COUNT; i++) {
        pthread_mutex_destroy(&backends[i].mutex);
    }

    uvhttp_server_free(server);

    return 0;
}
```

---

## Part Four: Production Practices

### Chapter 11: Performance Optimization

#### 11.1 Memory Optimization

```c
// Use a memory pool
typedef struct {
    void* pool;
    size_t block_size;
    size_t block_count;
    pthread_mutex_t mutex;
} memory_pool_t;

// Preallocate memory
memory_pool_t* create_memory_pool(size_t block_size, size_t block_count) {
    memory_pool_t* pool = malloc(sizeof(memory_pool_t));
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->pool = malloc(block_size * block_count);
    pthread_mutex_init(&pool->mutex, NULL);

    return pool;
}

void* memory_pool_alloc(memory_pool_t* pool) {
    pthread_mutex_lock(&pool->mutex);
    // Allocate memory from the pool
    pthread_mutex_unlock(&pool->mutex);
    return NULL;
}
```

#### 11.2 Connection Optimization

```c
// Enable Keep-Alive
uvhttp_config_t* config = uvhttp_config_new();
config->keepalive_timeout = 30; // 30 seconds
server->config = config;

// Connection reuse
// In a real application, implement connection pooling and reuse logic
```

---

### Chapter 12: Security Configuration

#### 12.1 TLS/SSL Configuration

```c
// Enable TLS
#if UVHTTP_FEATURE_TLS
uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
uvhttp_tls_context_load_cert(tls_ctx, "server.crt", "server.key");
uvhttp_server_enable_tls(server, tls_ctx);
#endif
```

#### 12.2 Setting Security Headers

```c
// Set secure response headers
int secure_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Content-Type-Options", "nosniff");
    uvhttp_response_set_header(res, "X-Frame-Options", "DENY");
    uvhttp_response_set_header(res, "X-XSS-Protection", "1; mode=block");
    uvhttp_response_set_header(res, "Strict-Transport-Security", "max-age=31536000");

    const char* json = "{\"message\":\"Secure response\"}";
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}
```

---

### Chapter 13: Monitoring and Logging

#### 13.1 Request Logging

```c
// Logging
void log_request(uvhttp_request_t* req, int status, size_t response_size) {
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    const char* user_agent = uvhttp_request_get_header(req, "User-Agent");

    printf("[%s] %s %s %d %zu \"%s\"\n",
           time_str, method, url, status, response_size,
           user_agent ? user_agent : "-");
}
```

#### 13.2 Performance Monitoring

```c
// Performance statistics
typedef struct {
    size_t total_requests;
    size_t total_bytes_sent;
    size_t total_bytes_received;
    double avg_response_time;
    pthread_mutex_t mutex;
} performance_stats_t;

static performance_stats_t g_stats = {0};

void update_stats(size_t bytes_sent, double response_time) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.total_requests++;
    g_stats.total_bytes_sent += bytes_sent;
    g_stats.avg_response_time =
        (g_stats.avg_response_time * (g_stats.total_requests - 1) + response_time) /
        g_stats.total_requests;
    pthread_mutex_unlock(&g_stats.mutex);
}
```

---

## Summary

This tutorial covers the complete learning path for UVHTTP, from beginner to expert:

1. **Getting started**: Hello World, core concepts, routing system
2. **Advanced development**: complex routing, request handling, response optimization
3. **Advanced architecture**: libuv data pointer, multithreading, asynchronous database, load balancing
4. **Production practices**: performance optimization, security configuration, monitoring and logging
5. **IoT communication**: real-time communication, device management, message push

### Next Steps

- Review the complete examples in the `examples/` directory
- Read `docs/API_REFERENCE.md` for the complete API
- Refer to `docs/ARCHITECTURE.md` for the architecture design
- Run the test suite with `make test`

### Best Practices Summary

1. **Use the core API**: avoid over-abstraction and use core functions directly
2. **Async first**: fully leverage libuv's asynchronous features
3. **Error handling**: check all return values and handle error cases
4. **Memory management**: use the unified allocator and avoid memory leaks
5. **Performance optimization**: configure connection counts, buffer sizes, and other parameters appropriately
6. **Security first**: enable TLS, set secure headers, and validate input
7. **Monitoring and logging**: log requests and monitor performance metrics
8. **Avoid global variables**: store application state with the libuv data pointer
9. **Thread safety**: use mutexes to protect shared data in multithreaded environments
10. **Context management**: follow RAII principles when creating and destroying contexts

---

## Appendix: Quick Reference

### A. Common Code Snippets

#### Building UVHTTP
```bash
# Clone the repository (including submodules)
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

> **Note**: the `--recurse-submodules` flag automatically clones all dependencies. If you forgot to use this flag, you can recover by running `git submodule update --init --recursive`.

# Build (using the project's bundled dependencies)
make build
```

#### Building Example Programs
```bash
# Build all examples
make build

# Build specific examples
make hello_world
make simple_routing

# Run examples
./examples/hello_world
./examples/simple_routing
```

#### Creating a Server

```c
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = NULL;
uvhttp_server_new(loop, &server);
uvhttp_router_t* router = NULL;
uvhttp_router_new(&router);
uvhttp_server_set_router(server, router);

uvhttp_router_add_route(router, "/", handler);
uvhttp_server_listen(server, "0.0.0.0", 8080);

uv_run(loop, UV_RUN_DEFAULT);
uvhttp_server_free(server);
```

#### Using an Application Context

```c
typedef struct {
    uvhttp_server_t* server;
    int count;
} app_context_t;

app_context_t* ctx = malloc(sizeof(app_context_t));
ctx->server = NULL;
uvhttp_server_new(loop, &ctx->server);
loop->data = ctx;

// Access it in a handler
app_context_t* ctx = (app_context_t*)loop->data;
ctx->count++;
```

#### JSON Response

```c
const char* json = "{\"message\":\"Hello\"}";
uvhttp_response_set_status(res, 200);
uvhttp_response_set_header(res, "Content-Type", "application/json");
uvhttp_response_set_body(res, json, strlen(json));
return uvhttp_response_send(res);
```

#### Error Handling

```c
int result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %d\n", result);
    // Clean up resources
    return 1;
}
```

### B. Configuration Parameters

| Parameter | Default | Description |
|------|--------|------|
| max_connections | 1000 | Maximum number of connections |
| max_body_size | 1048576 | Maximum request body size (1MB) |
| read_buffer_size | 8192 | Read buffer size |
| keepalive_timeout | 30 | Keep-Alive timeout (seconds) |
| request_timeout | 60 | Request timeout (seconds) |

### C. HTTP Status Codes

| Status Code | Meaning | Usage |
|--------|------|----------|
| 200 | OK | Success response |
| 201 | Created | Resource created successfully |
| 400 | Bad Request | Invalid request parameters |
| 401 | Unauthorized | Not authenticated |
| 403 | Forbidden | No permission |
| 404 | Not Found | Resource does not exist |
| 500 | Internal Server Error | Server error |

### D. Common Content-Types

| Type | Content-Type |
|------|--------------|
| JSON | application/json |
| HTML | text/html; charset=utf-8 |
| Plain text | text/plain; charset=utf-8 |
| XML | application/xml |
| CSS | text/css |
| JavaScript | application/javascript |
| WebSocket | websocket |
| Static files | auto-detected based on file extension |

### E. Static File Server Configuration

```c
// Configure the static file server
uvhttp_static_config_t static_config = {
    .root_directory = "./public",
    .index_file = "index.html",
    .enable_directory_listing = 1,
    .enable_etag = 1,
    .enable_last_modified = 1,
    .max_cache_size = 10 * 1024 * 1024,
    .cache_ttl = 3600
};

// Create the context
uvhttp_static_context_t* ctx = NULL;
uvhttp_static_create(&static_config, &ctx);

// Handle requests
uvhttp_static_handle_request(ctx, req, res);
```

### F. WebSocket Configuration

```c
// WebSocket handler
uvhttp_ws_handler_t ws_handler;
ws_handler.on_connect = ws_connect_handler;
ws_handler.on_message = ws_message_handler;
ws_handler.on_close = ws_close_handler;

// Register the handler
uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);

// Send a message
uvhttp_server_ws_send(ws_conn, data, len);

// Close the connection
uvhttp_server_ws_close(ws_conn, 1000, "Normal closure");
```

### G. Performance Optimization Recommendations

1. **Connection pooling**: reuse database connections
2. **Caching**: use an LRU cache to reduce database queries
3. **Compression**: enable response compression
4. **Asynchronous**: use asynchronous I/O to avoid blocking
5. **Load balancing**: process requests with multiple threads or processes
6. **Monitoring**: monitor performance metrics in real time
7. **Logging**: log critical operations and errors
8. **Static files**: enable file caching and ETag
9. **WebSocket**: use connection pooling to manage WebSocket connections
10. **Memory allocation**: use mimalloc to improve memory allocation performance

### H. Security Checklist

- [ ] Enable TLS/SSL
- [ ] Set secure response headers
- [ ] Validate all input
- [ ] Prevent SQL injection
- [ ] Prevent XSS attacks
- [ ] Limit request rates
- [ ] Use strong passwords
- [ ] Update dependencies regularly
- [ ] Enable audit logging
- [ ] Implement access control
- [ ] Validate static file paths (prevent directory traversal)
- [ ] Limit WebSocket message sizes
- [ ] Limit file upload sizes
- [ ] Use a file type whitelist

---

## Appendix: Dependency Management and Building

### Dependency Notes

UVHTTP uses a self-contained dependency management approach; all required dependencies are included in the `deps/` directory:

| Dependency | Directory | Purpose |
|------|------|------|
| libuv | `deps/libuv/` | Asynchronous I/O library, event loop core |
| llhttp | `deps/llhttp/` | HTTP parser |
| mbedtls | `deps/mbedtls/` | TLS/SSL support |
| cjson | `deps/cjson/` | JSON parsing and generation |
| mimalloc | `deps/mimalloc/` | Memory allocator |
| uthash | `deps/uthash/` | Hash table implementation |
| xxhash | `deps/xxhash/` | Fast hash algorithm |

### Compilation Options

**Debug build**:
```bash
make build
```

**Release build**:
```bash
make build
```

**Enable specific features**:

Edit the `option()` defaults in `CMakeLists.txt`, then run `make build`:

```bash
# Enable TLS support — set UVHTTP_FEATURE_TLS to ON in CMakeLists.txt
# Enable WebSocket support — set UVHTTP_FEATURE_WEBSOCKET to ON in CMakeLists.txt
# Disable mimalloc (use the system malloc) — set UVHTTP_HAS_MIMALLOC to OFF in CMakeLists.txt
make build
```

### Building Example Programs

**Using CMake to build a single example**:
```bash
# In the project root directory
make build

# Build specific examples
make hello_world
make simple_routing
make method_routing

# Or build all examples
make examples

# Run examples
./examples/hello_world
./examples/simple_routing
./examples/method_routing
```

**Manually create CMakeLists.txt (optional)**:
```bash
# Create CMakeLists.txt in the examples/ directory
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# Find UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)
find_library(UVHTTP_LIBRARY uvhttp PATHS ../build NO_DEFAULT_PATH)

include_directories(${UVHTTP_INCLUDE_DIR})

# Add examples
add_subdirectory(01_basics)
add_subdirectory(02_routing)
EOF

# In 01_basics/CMakeLists.txt
cat > 01_basics/CMakeLists.txt << 'EOF'
add_executable(hello_world 01_hello_world.c)
target_link_libraries(hello_world ${UVHTTP_LIBRARY} uv pthread m)
EOF
```

**Quick build script**:
```bash
#!/bin/bash
# build_example.sh
EXAMPLE_NAME=$1

cd build
make build > /dev/null 2>&1
make $EXAMPLE_NAME

if [ $? -eq 0 ]; then
    echo "Build succeeded: $EXAMPLE_NAME"
    echo "Run: ./examples/$EXAMPLE_NAME"
else
    echo "Build failed: $EXAMPLE_NAME"
fi
```

Usage:
```bash
chmod +x build_example.sh
./build_example.sh hello_world
```

### Common Build Issues

**Issue 1: header file not found**
```bash
error: uvhttp.h: No such file or directory
```
**Solution**: make sure the include path is correct
```bash
gcc -I../include ...
```

**Issue 2: link error**
```bash
undefined reference to `uvhttp_server_new'
```
**Solution**: link the UVHTTP library
```bash
gcc -L../build -luvhttp ...
```

**Issue 3: library not found at runtime**
```bash
error while loading shared libraries: libuvhttp.so
```
**Solution**: set the library path
```bash
export LD_LIBRARY_PATH=../build:$LD_LIBRARY_PATH
```

---

## Appendix: In-Application Load Balancing Supplement

### In-Application Load Balancing vs. External Gateway

**In-application load balancing** (recommended for simple scenarios):
- ✅ No extra components needed
- ✅ Fewer network hops
- ✅ Simpler deployment
- ✅ Lower latency
- ❌ Relatively simple functionality
- ❌ Limited scalability

**External load balancing** (recommended for production environments):
- ✅ Powerful
- ✅ Easy to scale
- ✅ Supports multiple algorithms
- ❌ Requires additional deployment
- ❌ Adds network latency
- ❌ More complex operations

### In-Application Load Balancing Implementation

**Multithreaded worker pool pattern**:
```c
#define WORKER_THREADS 4

// Worker thread context
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    int request_count;
    pthread_mutex_t mutex;
} worker_context_t;

// Select a worker thread (round-robin)
int select_worker() {
    static int current = 0;
    return (current++) % WORKER_THREADS;
}

// Dispatch requests from the main thread
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    int worker_id = select_worker();

    // Send the request info to the worker thread for processing
    // A real implementation needs an inter-thread communication mechanism

    return 0;
}
```

**Single-threaded event loop + libuv thread pool**:
```c
// Use libuv's thread pool
void process_in_thread_pool(uv_work_t* req) {
    // Execute the time-consuming operation in the thread pool
}

void after_thread_pool(uv_work_t* req, int status) {
    // Process the result in the main thread
}

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = req;

    uv_queue_work(loop, work_req, process_in_thread_pool, after_thread_pool);

    return 0;
}
```

### Choosing a Load Balancing Algorithm

| Algorithm | Use Case | Advantages | Disadvantages |
|------|----------|------|------|
| Round-robin | Similar requests | Simple, fair | Does not consider load differences |
| Least connections | Varying request durations | Good load balancing | Requires maintaining connection counts |
| IP hash | Needs session affinity | Same IP goes to same server | Can be unbalanced |
| Random | Simple scenarios | Simple | Can be unbalanced |

---

## Related Resources

### Official Documentation
- [API Reference](../api/API_REFERENCE.md)
- [Architecture Design](../dev/ARCHITECTURE.md)
- [Developer Guide](DEVELOPER_GUIDE.md)
- [libuv Data Pointer](LIBUV_DATA_POINTER.md)

### Example Programs
- [Basic examples](https://github.com/adam-ikari/uvhttp/tree/main/examples/01_basics)
- [Routing examples](https://github.com/adam-ikari/uvhttp/tree/main/examples/02_routing)
- [Advanced examples](https://github.com/adam-ikari/uvhttp/tree/main/examples/06_advanced)

### External Resources
- [libuv official documentation](https://docs.libuv.org/)
- [HTTP/1.1 specification](https://tools.ietf.org/html/rfc7231)
- [C++ Core Guidelines best practices](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

---

Build HTTP servers with UVHTTP.
