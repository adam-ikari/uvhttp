# Quick Start

## Requirements

- CMake 3.10+
- C11 compiler (GCC 4.9+, Clang 3.5+, MSVC 2015+)
- libuv 1.x
- llhttp

## Installation

### Build from Source

```bash
# Clone repository
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Create build directory
mkdir build && cd build

# Configure project
cmake ..

# Build
make -j$(nproc)

# Run tests (optional)
make test
```

### Install Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential libuv1-dev
```

#### macOS

```bash
brew install cmake libuv
```

#### Windows

Install dependencies using vcpkg:

```bash
vcpkg install libuv
```

## First Program

Create `hello.c`:

```c
#include <uvhttp.h>
#include <stdio.h>

void hello_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello, World!");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    // Add route
    uvhttp_router_add_route(router, "/", hello_handler);

    // Start server
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### Compile

```bash
gcc -o hello hello.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
```

### Run

```bash
./hello
```

Visit http://localhost:8080 to see "Hello, World!".

## Project Structure

```
uvhttp/
├── include/           # Public headers
├── src/              # Source code
├── docs/             # Documentation
├── examples/         # Example programs
├── test/             # Tests
└── build/            # Build output directory
```

## Configuration Options

```bash
# Enable WebSocket support
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# Enable mimalloc allocator
cmake -DBUILD_WITH_MIMALLOC=ON ..

# Debug mode
cmake -DENABLE_DEBUG=ON ..

# Enable code coverage
cmake -DENABLE_COVERAGE=ON ..

# Enable example compilation
cmake -DBUILD_EXAMPLES=ON ..
```

## Next Steps

- [API Documentation](/en/api/introduction) - Learn the complete API
- [Routing](/en/guide/routing) - Learn how to use routing
- [Middleware](/en/guide/middleware) - Use middleware features
- [WebSocket](/en/guide/websocket) - WebSocket support