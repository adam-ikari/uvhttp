---
title: Quick Start
description: Get started with UVHTTP in minutes — build the C99 HTTP/WebSocket server library with CMake and Make, run the hello-world example, and add your first route.
---

# Quick Start

You can have a server running in a few minutes: build the library, compile one C file, add one route.

## Platform Support

**Current support**: Linux

**Future plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

> **Note**: UVHTTP supports Linux only right now. Support for other operating systems is planned for later releases.

## Prerequisites

- CMake 3.13 or higher
- C99 compiler (GCC 4.9+, Clang 3.5+)
- libuv 1.x (cloned automatically as a submodule)
- llhttp (cloned automatically as a submodule)

## Installation

### Install Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential
```

### Clone and Build

```bash
# Clone the repository (including submodules)
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Build the library
make build

# Run tests (optional)
make test
```

> **Note**: The `--recurse-submodules` flag is what pulls in libuv, llhttp and the rest. If you forget it, run `git submodule update --init --recursive` afterward.

## Your First Server

Create a file `hello.c`:

```c
#include <uvhttp.h>
#include <stdio.h>
#include <string.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", strlen("Hello, World!"));
    return uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_take_router(server, router);

    // Add a route
    uvhttp_router_add_route(router, "/", hello_handler);

    // Start the server
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

The handler sets the status, the content type and the body, then calls uvhttp_response_send and returns its result. uvhttp_server_take_router hands the router to the server, which owns it from then on.

### Compile and Run

```bash
gcc -o hello hello.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
./hello
```

The -I flag points at the public headers and -L at the built library under build/dist/lib.

Visit `http://localhost:8080` to see "Hello, World!". If the port is already taken, the program exits with the error string from uvhttp_server_listen.

## Project Structure

```
uvhttp/
├── include/           # Public headers
├── src/               # Source implementation
├── docs/              # Documentation
├── examples/          # Example programs
├── test/              # Tests
└── build/             # Build output directory
```

## Configuration Options

Build options are the `option()` entries in `CMakeLists.txt`. Set them when you configure the build, then rebuild:

```bash
# Rebuild after editing CMakeLists.txt options
make build
```

## Next Steps

- [API Reference](../api/API_REFERENCE.md) - Learn the complete API
- [Tutorial](./TUTORIAL.md) - Build a complete application
- [WebSocket support](./websocket.md) - Real-time messaging

## Need Help?

- Check the [documentation](../)
- Open an [issue](https://github.com/adam-ikari/uvhttp/issues)
- Join the discussions on GitHub
