# Quick Start

This guide will help you get started with UVHTTP in just a few minutes.

## Prerequisites

- C compiler (GCC or Clang)
- CMake 3.10 or higher
- libuv (will be downloaded automatically)

## Installation

### Clone the Repository

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### Build the Library

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run Tests

```bash
./run_tests.sh
```

## Your First Server

Create a file `hello.c`:

```c
#include <uvhttp.h>
#include <uv_loop.h>

int main() {
    // Create event loop
    uv_loop_t* loop = uv_default_loop();
    
    // Create server
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // Create router
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    // Add a route handler
    uvhttp_router_add_route(router, "/hello", [](uvhttp_request_t* req) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        
        // Set status code
        uvhttp_response_set_status(res, 200);
        
        // Set headers
        uvhttp_response_set_header(res, "Content-Type", "text/plain");
        
        // Set body
        uvhttp_response_set_body(res, "Hello, World!");
        
        // Send response
        uvhttp_response_send(res);
    });
    
    // Start server
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    printf("Server running on http://0.0.0.0:8080\n");
    
    // Run event loop
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

### Compile and Run

```bash
gcc hello.c -Iinclude -Lbuild/dist/lib -luvhttp -lpthread -luv -o hello
./hello
```

Visit `http://localhost:8080/hello` in your browser!

## Next Steps

- Learn about [routing](../guide/TUTORIAL.md)
- Explore [WebSocket support](../guide/websocket.md)
- Check out [API reference](../api/API_REFERENCE.md)

## Need Help?

- Check the [documentation](../)
- Open an [issue](https://github.com/adam-ikari/uvhttp/issues)
- Join the discussions on GitHub