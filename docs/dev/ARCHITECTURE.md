# UVHTTP Architecture Design Document

## Overview

UVHTTP is an HTTP/1.1 server library built on libuv, designed with a modular architecture.

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

**Responsibilities**:
- Server lifecycle management
- Connection acceptance and management
- Request dispatch

**Key Data Structures**:
```c
typedef struct uvhttp_server {
    uv_loop_t* loop;
    uv_tcp_t tcp_handle;
    uvhttp_router_t* router;
    uvhttp_context_t* context;
    // ... other fields
} uvhttp_server_t;
```

**Core Flow**:
1. Create the server object
2. Bind address and port
3. Listen for connections
4. Accept new connections
5. Dispatch requests to the router

### 2. Router Module (uvhttp_router)

**Responsibilities**:
- URL path matching
- HTTP method routing
- Middleware chain management

**Routing Match Algorithm**:
- Prefix matching (O(1))
- Wildcard support
- Parameter extraction support

### 3. Connection Module (uvhttp_connection)

**Responsibilities**:
- Connection lifecycle management
- Read/write buffer management
- HTTP parsing

**State Machine**:
```
NEW → TLS_HANDSHAKE → HTTP_READING → HTTP_PROCESSING → HTTP_WRITING → CLOSING
```

**Memory Layout Optimization**:
- Hot-path fields first (frequently accessed)
- Pointer fields 8-byte aligned
- Large buffers at the end of the struct

### 4. Request/Response Module (uvhttp_request/uvhttp_response)

**Responsibilities**:
- HTTP request/response parsing
- Header management
- Body handling

**Zero-Copy Optimization**:
- Uses libuv buffers
- Avoids data copying
- sendfile support

## Memory Management

### Allocator Design Principles

UVHTTP uses a **compile-time optimized** memory management strategy: the allocator is selected via a compile-time macro, achieving zero runtime overhead.

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

**Characteristics**:
- Stable and reliable, no additional dependencies
- Zero abstraction overhead
- Suitable for most use cases

#### 2. mimalloc Allocator

```c
static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}
```

**Characteristics**:
- High-performance, modern allocator
- Built-in small-object optimization
- Better multi-threading scalability
- Reduced memory fragmentation

### Build Configuration

Select the allocator via the CMake build macro:

```cmake
# System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc allocator
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### Usage

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

- **Zero runtime overhead**: all functions are inline
- **Compile-time optimization**: the compiler can fully optimize
- **Type safety**: compile-time type checking
- **Predictability**: no dynamic dispatch

### Best Practices

1. **Use a single allocator consistently**: always use `uvhttp_alloc/uvhttp_free`, do not mix with `malloc/free`
2. **Pair allocations and frees**: every allocation has a corresponding free
3. **Check return values**: verify whether the allocation succeeded
4. **Avoid leaks**: ensure memory is freed on all code paths

## Error Handling

### Error Code Design

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

1. **Check all function calls that can fail**
2. **Use a unified error type**
3. **Provide meaningful error messages**
4. **Support error recovery**

## Performance Optimization

### 1. Zero-Copy Optimization

- Uses libuv buffers
- sendfile support
- Avoids data copying

### 2. Caching Strategy

- LRU cache for static files
- Route caching
- Connection reuse

### 3. Memory Optimization

- Inline functions
- Compile-time optimization
- Memory pooling (optional)

### 4. I/O Optimization

- Asynchronous, non-blocking I/O
- Batch operations
- Zero-copy transfer

## Security Features

### 1. Input Validation

- Parameter checking
- Boundary checking
- Type validation

### 2. Memory Safety

- Boundary checking
- Double-free detection
- Use-after-free protection

### 3. Network Security

- TLS support (mbedtls)
- Configuration validation
- Error handling

## Extensibility

### 1. Middleware System

- Pre-request processing
- Post-request processing
- Custom middleware

### 2. Plugin System

- Custom routing
- Custom handlers
- Custom allocators

### 3. Configuration System

- Runtime configuration
- Compile-time configuration
- Environment variable configuration

## Testing Strategy

### 1. Unit Tests

- Test individual functions
- Test boundary conditions
- Test error handling

### 2. Integration Tests

- Test module interactions
- Test end-to-end flows
- Test performance

### 3. Performance Tests

- Benchmark tests
- Stress tests
- Memory analysis

## Documentation Structure

```
docs/
├── api/                    # API documentation
│   └── API_REFERENCE.md
├── dev/                    # Contributor documentation
│   ├── ARCHITECTURE.md     # Architecture design (this document)
│   ├── DEVELOPER_GUIDE.md  # Developer guide
│   └── ROADMAP.md          # Roadmap
└── guide/                  # User guide
    ├── TUTORIAL.md         # Tutorial
    └── DEVELOPER_GUIDE.md  # Developer guide
```

## Version History

- **v2.0.0**: Refactored architecture, removed abstraction layers
- **v1.4.0**: Added WebSocket support
- **v1.3.0**: Added TLS support
- **v1.2.0**: Added static file serving
- **v1.1.0**: Added routing functionality
- **v1.0.0**: Initial version

## References

- [libuv documentation](https://docs.libuv.org/)
- [HTTP/1.1 specification](https://tools.ietf.org/html/rfc7230)
- [WebSocket specification](https://tools.ietf.org/html/rfc6455)
