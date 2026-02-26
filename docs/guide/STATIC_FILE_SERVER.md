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

## Cache Prewarming Strategies

UVHTTP provides flexible cache prewarming APIs that allow the application layer to implement various optimization strategies based on specific use cases. This follows the principle of "application layer control" - the framework provides the infrastructure, and the application decides the strategy.

### Available Prewarming APIs

```c
// Prewarm a single file
uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,
                                            const char* file_path);

// Prewarm an entire directory
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path, int max_files);

// Direct cache prewarming (low-level)
uvhttp_error_t uvhttp_lru_cache_prewarm(cache_manager_t* cache,
                                        const char* file_path,
                                        const char* content,
                                        size_t content_length,
                                        const char* mime_type);
```

### Strategy 1: Directory-Based Prewarming by File Type

Preload common web asset types from a directory:

```c
int prewarm_web_assets(uvhttp_static_context_t* ctx, const char* dir_path) {
    const char* web_extensions[] = {".css", ".js", ".png", ".jpg", ".svg", ".woff2"};
    int prewarmed_count = 0;

    DIR* dir = opendir(dir_path);
    if (!dir) return -1;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && prewarmed_count < 100) {
        const char* ext = strrchr(entry->d_name, '.');
        if (!ext) continue;

        // Check if extension matches web assets
        for (size_t i = 0; i < sizeof(web_extensions) / sizeof(web_extensions[0]); i++) {
            if (strcasecmp(ext, web_extensions[i]) == 0) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                
                if (uvhttp_static_prewarm_cache(ctx, full_path) == UVHTTP_OK) {
                    prewarmed_count++;
                }
                break;
            }
        }
    }

    closedir(dir);
    return prewarmed_count;
}

// Usage
prewarm_web_assets(static_ctx, "./public/static");
```

### Strategy 2: Preload Related Files from HTML

Parse HTML to extract and preload referenced resources:

```c
int preload_html_resources(uvhttp_static_context_t* ctx, const char* html_path) {
    // Read HTML file
    FILE* f = fopen(html_path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* html = malloc(size + 1);
    fread(html, 1, size, f);
    html[size] = '\0';
    fclose(f);

    // Extract resource paths (simplified example)
    char* patterns[] = {"href=\"", "src=\""};
    int prewarmed_count = 0;

    for (int i = 0; i < 2 && prewarmed_count < 50; i++) {
        char* p = html;
        while ((p = strstr(p, patterns[i])) != NULL && prewarmed_count < 50) {
            p += strlen(patterns[i]);
            char* end = strchr(p, '"');
            if (!end) break;

            *end = '\0';
            char resource_path[512];
            snprintf(resource_path, sizeof(resource_path), "./public/%s", p);

            if (uvhttp_static_prewarm_cache(ctx, resource_path) == UVHTTP_OK) {
                prewarmed_count++;
            }

            *end = '"';
            p = end + 1;
        }
    }

    free(html);
    return prewarmed_count;
}

// Usage
preload_html_resources(static_ctx, "./public/index.html");
```

### Strategy 3: Priority-Based Prewarming

Set different priorities for different file types:

```c
void prewarm_with_priority(uvhttp_static_context_t* ctx) {
    // High priority: Core CSS and JS
    const char* high_priority_files[] = {
        "./public/css/main.css",
        "./public/js/app.js",
        "./public/js/vendor.js"
    };

    for (size_t i = 0; i < sizeof(high_priority_files) / sizeof(high_priority_files[0]); i++) {
        uvhttp_lru_cache_set_entry_priority(ctx->cache, high_priority_files[i], 100);
        uvhttp_static_prewarm_cache(ctx, high_priority_files[i]);
    }

    // Medium priority: Images
    const char* medium_priority_files[] = {
        "./public/images/logo.png",
        "./public/images/banner.jpg"
    };

    for (size_t i = 0; i < sizeof(medium_priority_files) / sizeof(medium_priority_files[0]); i++) {
        uvhttp_lru_cache_set_entry_priority(ctx->cache, medium_priority_files[i], 50);
        uvhttp_static_prewarm_cache(ctx, medium_priority_files[i]);
    }
}
```

### Strategy 4: Gradual Prewarming on Server Start

Warm up cache gradually during server initialization:

```c
void gradual_prewarm(uvhttp_static_context_t* ctx, const char* dir_path) {
    int batch_size = 10;
    int total_files = 0;
    int prewarmed = 0;

    // First batch: Core files
    prewarmed = uvhttp_static_prewarm_directory(ctx, dir_path, batch_size);
    printf("Prewarmed %d core files\n", prewarmed);

    // Second batch: Additional files
    prewarmed = uvhttp_static_prewarm_directory(ctx, dir_path, batch_size);
    printf("Prewarmed %d additional files\n", prewarmed);

    // Continue until cache is full or all files are loaded
    size_t total_memory, entry_count;
    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory, &entry_count, NULL);
    printf("Cache stats: %zu bytes, %d entries\n", total_memory, entry_count);
}
```

### Strategy 5: On-Demand Prewarming

Preload files when they are first requested:

```c
int smart_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    app_context_t* app_ctx = (app_context_t*)req->client->loop->data;

    // Handle the current request
    int result = uvhttp_static_handle_request(app_ctx->static_ctx, req, res);

    // If this is a CSS or JS file, preload related files
    if (result == UVHTTP_OK && req->path) {
        if (strstr(req->path, ".css") || strstr(req->path, ".js")) {
            char base_path[512];
            strncpy(base_path, req->path, sizeof(base_path));
            char* last_slash = strrchr(base_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                // Prewarm other files in the same directory
                uvhttp_static_prewarm_directory(app_ctx->static_ctx, base_path, 5);
            }
        }
    }

    return result;
}
```

### Performance Monitoring

Monitor cache effectiveness:

```c
void print_cache_stats(uvhttp_static_context_t* ctx) {
    size_t total_memory;
    int entry_count;
    double hit_rate;

    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory, &entry_count, NULL);
    hit_rate = uvhttp_lru_cache_get_hit_rate(ctx->cache);

    printf("Cache Statistics:\n");
    printf("  Memory Usage: %zu bytes\n", total_memory);
    printf("  Entry Count: %d\n", entry_count);
    printf("  Hit Rate: %.2f%%\n", hit_rate * 100);
}
```

### Best Practices for Prewarming

1. **Start Simple**: Begin with directory-based prewarming before implementing complex strategies
2. **Monitor Memory**: Track cache usage to avoid excessive memory consumption
3. **Prioritize**: Set higher priorities for frequently accessed files
4. **Be Selective**: Don't prewarm everything - focus on critical resources
5. **Profile**: Use cache statistics to identify hot files and optimize prewarming strategy

### When to Use Prewarming

- **Production Deployment**: Preload critical assets before accepting traffic
- **Zero-Downtime Deployments**: Warm up new instances before routing traffic
- **High-Traffic Events**: Prepare cache for expected traffic spikes
- **Performance Tuning**: Optimize based on access patterns

### When NOT to Use Prewarming

- **Development**: Unnecessary during development
- **Small-Scale Apps**: Cache may not provide significant benefit
- **Dynamically Generated Content**: Static cache won't help
- **Memory-Constrained Environments**: May cause memory pressure

## See Also

- [FAQ](../FAQ.md) - Common questions and answers
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Performance Guide](./zh/dev/PERFORMANCE_BENCHMARK.md) - Performance benchmarks