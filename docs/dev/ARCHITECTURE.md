# UVHTTP Architecture Design Document

## Overview

UVHTTP is an HTTP/1.1 server library built on libuv. The code separates protocol handling, routing, and connection management into modules with narrow responsibilities.

## Core Architecture

### Layered Architecture

```
┌─────────────────────────────────────────┐
│         Application Layer              │
├─────────────────────────────────────────┤
│         API Layer                       │
│  uvhttp_server, uvhttp_router, etc.     │
├─────────────────────────────────────────┤
│       Business Logic Layer              │
│  Request handling, routing, middleware, │
│         static files                    │
├─────────────────────────────────────────┤
│       Core Services Layer               │
│  Connection management, error handling, │
│           memory management             │
├─────────────────────────────────────────┤
│        Network Layer                    │
│     libuv (asynchronous I/O)            │
└─────────────────────────────────────────┘
```

### Module Dependencies

```
uvhttp_server
    ├── uvhttp_router
    ├── uvhttp_connection
    ├── uvhttp_request
    ├── uvhttp_response
    └── uvhttp_context

uvhttp_connection
    ├── uvhttp_request
    ├── uvhttp_response
    └── libuv

uvhttp_static
    ├── uvhttp_response
    └── uvhttp_lru_cache

uvhttp_websocket
    ├── uvhttp_connection
    └── uvhttp_response
```

## Core Modules

### 1. Server Module (uvhttp_server)

The server module owns the server lifecycle, accepts and tracks connections, and dispatches requests.

Its main structure holds the loop, the listening TCP handle, the router, and the context:

```c
typedef struct uvhttp_server {
    uv_loop_t* loop;
    uv_tcp_t tcp_handle;
    uvhttp_router_t* router;
    uvhttp_context_t* context;
    // ... other fields
} uvhttp_server_t;
```

Startup follows five steps:

1. Create the server object
2. Bind address and port
3. Listen for connections
4. Accept new connections
5. Dispatch requests to the router

### 2. Router Module (uvhttp_router)

The router matches URL paths, routes by HTTP method, and manages the middleware chain.

Matching is prefix-based and runs in O(1). Routes support wildcards and path parameter extraction.

### 3. Connection Module (uvhttp_connection)

A connection has three jobs: manage its lifecycle, manage the read and write buffers, and parse HTTP.

The connection moves through this state machine:

```
NEW → TLS_HANDSHAKE → HTTP_READING → HTTP_PROCESSING → HTTP_WRITING → CLOSING
```

The struct keeps hot-path fields first, aligns pointer fields to 8 bytes, and puts large buffers at the end.

### 4. Request/Response Module (uvhttp_request/uvhttp_response)

These two objects parse incoming requests and outgoing responses, manage headers, and handle bodies.

Request and response handling avoids copies. It uses libuv buffers, does not duplicate the payload, and supports sendfile.

## Memory Management

### Allocator Design Principles

UVHTTP chooses the allocator at compile time with a macro. The choice costs nothing at runtime, because the allocation helpers inline down to plain malloc or mi_malloc calls.

### Allocator Types

#### 1. System Allocator (default)

```c
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    free(ptr);
}
```

The system allocator is stable, adds no dependency, and adds no abstraction layer. It fits most applications.

#### 2. mimalloc Allocator

```c
static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}
```

mimalloc is a modern, high-performance allocator. It optimizes small objects, scales better across threads, and fragments memory less.

### Build Configuration

Pick the allocator with a CMake variable:

```cmake
# System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc allocator
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### Usage

The allocation API covers the four operations you need:

```c
// Allocate memory
void* ptr = uvhttp_alloc(size);

// Reallocate
ptr = uvhttp_realloc(ptr, new_size);

// Free memory
uvhttp_free(ptr);

// Allocate and initialize
ptr = uvhttp_calloc(count, size);
```

### Performance Characteristics

All four functions are inline, so the compiler sees through them and optimizes the call site. Type checking still happens at compile time, and there is no dynamic dispatch.

### Best Practices

1. Keep one allocator for the whole program. Use `uvhttp_alloc/uvhttp_free` for everything and never mix them with `malloc/free`.
2. Pair every allocation with a free.
3. Check the return value of each allocation.
4. Free on every path out of a function so nothing leaks.

## Error Handling

### Error Code Design

Every failure shares one enum:

```c
typedef enum {
    UVHTTP_OK = 0,                           /* Success */
    UVHTTP_ERROR_INVALID_PARAM = -1,        /* Invalid parameter */
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,       /* Out of memory */
    UVHTTP_ERROR_IO = -3,                   /* I/O error */
    // ... more error codes
} uvhttp_error_t;
```

### Error Handling Principles

1. Check every call that can fail.
2. Return the shared error type.
3. Put enough detail in the message to identify the cause.
4. Give the caller a way to recover.

## Performance Optimization

### 1. Zero-Copy Optimization

Payloads move through libuv buffers and sendfile, so the data is not copied twice.

### 2. Caching Strategy

Static files go through an LRU cache. Route lookups are cached, and connections are reused.

### 3. Memory Optimization

Helpers are inline, most configuration is resolved at compile time, and memory pooling is available as an option.

### 4. I/O Optimization

I/O is asynchronous and non-blocking. The server batches operations and moves payloads with zero-copy transfer.

## Security Features

### 1. Input Validation

Parameters, bounds, and types are checked before use.

### 2. Memory Safety

The library checks bounds, detects double frees, and protects against use-after-free.

### 3. Network Security

TLS support comes from mbedtls. The server validates its configuration and reports failures through the error codes.

## Extensibility

### 1. Middleware System

Middleware runs before and after request handling, and applications can add their own.

### 2. Plugin System

Applications supply their own routing, handlers, and allocators.

### 3. Configuration System

Configuration can be set at compile time, at runtime, or through environment variables.

## Testing Strategy

### 1. Unit Tests

Unit tests cover individual functions, boundary conditions, and error handling.

### 2. Integration Tests

Integration tests exercise module interactions, end-to-end flows, and performance.

### 3. Performance Tests

The suite includes benchmarks, stress tests, and memory analysis.

## Documentation Structure

```
docs/
├── api/                    # Generated API docs + handwritten API reference
│   ├── API_REFERENCE.md
│   └── generated/
├── dev/                    # Contributor documentation
│   ├── index.md            # Developer documentation hub
│   ├── ARCHITECTURE.md     # Architecture design (this document)
│   ├── BUILD_MODES.md
│   ├── CI_CD.md
│   ├── DEPENDENCIES.md
│   └── TESTING_STANDARDS.md
├── guide/                  # User and contributor guides
│   ├── DEVELOPER_GUIDE.md
│   ├── TESTING_GUIDE.md
│   └── TUTORIAL.md
└── zh/                     # Chinese mirror of the same structure
```

## Version History

- v3.0.0 removed the built-in rate limiter; rate limiting moved into the pre-dispatch request filter.
- v2.0.0 refactored the architecture and removed the abstraction layers.
- v1.4.0 added WebSocket support.
- v1.3.0 added TLS support.
- v1.2.0 added static file serving.
- v1.1.0 added routing.
- v1.0.0 was the first release.

## References

- [libuv documentation](https://docs.libuv.org/)
- [HTTP/1.1 specification](https://tools.ietf.org/html/rfc7230)
- [WebSocket specification](https://tools.ietf.org/html/rfc6455)
