# UVHTTP Static File Server Guide

## Overview

UVHTTP static file server is a high-performance, secure, and easy-to-use solution for serving static files. It provides comprehensive static file serving capabilities including automatic MIME type detection, file caching, conditional request support, and more.

## Design Principles

### Application Layer Implementation
Static file routing should be implemented by the application layer, not built into the framework. This follows UVHTTP's "Focus on Core" design principle:

- **Framework Core**: Provides `uvhttp_static_handle_request()` function to handle individual static file requests
- **Application Layer**: Responsible for routing configuration, path mapping, context passing, etc.
- **Flexibility**: Application layer has complete control over static file service routing strategies

### Recommended Implementation
```c
// 1. Create static file context
uvhttp_static_context_t* static_ctx;
uvhttp_static_create(&config, &static_ctx);

// 2. Create application layer wrapper function
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // Get static_ctx from context
    app_context_t* app_ctx = (app_context_t*)loop->data;
    return uvhttp_static_handle_request(app_ctx->static_ctx, request, response);
}

// 3. Add routes (application layer control)
uvhttp_router_add_route(router, "/static/*", static_file_handler);
uvhttp_router_add_route(router, "/*", static_file_handler);  // Fallback route
```

### Why Not Built-in?
- Avoid framework bloat
- Maintain application layer flexibility and control
- Follow the "Less is More" minimalist engineering principle

## Core Features

### Performance Optimization
- **LRU Cache System**: Intelligent memory caching reduces disk I/O
- **Zero-Copy Optimization**: Efficient file transfer mechanism
- **Connection Reuse**: libuv-based event-driven architecture
- **Compression Support**: Reserved gzip/deflate compression interface

### Security Features
- **Path Safety Validation**: Prevents directory traversal attacks
- **File Type Checking**: Configurable file type whitelist
- **Access Control**: Path-based access restriction support
- **Resource Limits**: Prevents large file DoS attacks

### Functional Features
- **Automatic MIME Type Detection**: Support for common file types
- **Conditional Requests**: ETag and Last-Modified support
- **Directory Listing**: Configurable directory browsing
- **Custom Headers**: Support for adding custom HTTP headers
- **Error Handling**: Friendly error pages and logging

## Quick Start

### Basic Example

See `examples/04_static_files/static_file_server.c` for a complete example demonstrating best practices for static file routing.

### Key Points
- Use `uvhttp_router_add_route()` to add static file routes
- Create wrapper functions to call `uvhttp_static_handle_request()`
- Pass application context through `server->context` or `loop->data`
- Use wildcard routes to handle multiple static file paths

## API Reference

For detailed API documentation, see:
- `include/uvhttp_static.h` - Static file service API
- `examples/04_static_files/` - Example programs

## Best Practices

1. **Route Configuration**
   - Use specific routes when possible (e.g., `/static/*`)
   - Place wildcard routes last to avoid blocking other routes
   - Consider separating static and dynamic content

2. **Security**
   - Always validate file paths
   - Configure file size limits
   - Use proper directory permissions

3. **Performance**
   - Enable LRU cache for frequently accessed files
   - Use appropriate cache TTL values
   - Consider CDN for large-scale deployments

## See Also

- [FAQ](../FAQ.md) - Common questions and answers
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Performance Guide](../dev/PERFORMANCE_BENCHMARK.md) - Performance benchmarks