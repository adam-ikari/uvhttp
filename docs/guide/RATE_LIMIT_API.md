# Rate Limit API Documentation

## Overview

UVHTTP provides server-level rate limiting to prevent DDoS attacks and overload. The rate limiting feature can be enabled or disabled at compile time through macros.

## Features

- **Server-level rate limiting**: All requests share the same rate limit counter
- **Fixed window algorithm**: A simple and efficient rate limiting algorithm
- **IP whitelist**: Supports an IP address whitelist that is exempt from rate limiting
- **Zero overhead**: Implemented via conditional compilation; no runtime overhead when disabled
- **Configurable**: Supports custom maximum request count and time window

## Compile Configuration

### Enable Rate Limiting (default)

```bash
make build
```

### Disable Rate Limiting

Edit `CMakeLists.txt`, change the default value of `option(UVHTTP_FEATURE_RATE_LIMIT ...)` to `OFF`, then run:

```bash
make build
```

## API Reference

### Enable Rate Limiting

```c
uvhttp_error_t uvhttp_server_enable_rate_limit(
    uvhttp_server_t* server,
    int max_requests,
    int window_seconds
);
```

**Parameters:**

- `server`: The server instance
- `max_requests`: Maximum number of requests allowed within the time window (range: 1-1000000)
- `window_seconds`: The time window in seconds (range: 1-86400)

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter

**Description:**

- Implements rate limiting using the fixed window algorithm
- Rate limit state is embedded directly in the server structure, requiring no dynamic memory allocation
- All requests share the same rate limit counter (server-level rate limiting)

**Example:**

```c
// At most 1000 requests per second
uvhttp_server_enable_rate_limit(server, 1000, 1);

// At most 6000 requests per minute
uvhttp_server_enable_rate_limit(server, 6000, 60);
```

### Disable Rate Limiting

```c
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t* server);
```

**Parameters:**

- `server`: The server instance

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter

**Example:**

```c
uvhttp_server_disable_rate_limit(server);
```

### Add an IP Whitelist Entry

```c
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(
    uvhttp_server_t* server,
    const char* client_ip
);
```

**Parameters:**

- `server`: The server instance
- `client_ip`: The client IP address (e.g., "127.0.0.1")

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter
- `UVHTTP_ERROR_OUT_OF_MEMORY`: Memory allocation failed

**Example:**

```c
// The loopback address is not rate limited
uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");

// The internal network address is not rate limited
uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1");
```

### Get Rate Limit Status

```c
uvhttp_error_t uvhttp_server_get_rate_limit_status(
    uvhttp_server_t* server,
    const char* client_ip,
    int* remaining,
    uint64_t* reset_time
);
```

**Parameters:**

- `server`: The server instance
- `client_ip`: The client IP address (currently unused, reserved for future expansion)
- `remaining`: Remaining request count (output parameter)
- `reset_time`: Reset timestamp in milliseconds (output parameter)

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter

**Note:**

- The current implementation uses server-level rate limiting; the `client_ip` parameter is unused
- The returned `remaining` and `reset_time` are server-level state

**Example:**

```c
int remaining;
uint64_t reset_time;
uvhttp_server_get_rate_limit_status(server, "127.0.0.1", &remaining, &reset_time);
printf("Remaining requests: %d, reset time: %lu\n", remaining, reset_time);
```

### Reset a Client's Rate Limit State

```c
uvhttp_error_t uvhttp_server_reset_rate_limit_client(
    uvhttp_server_t* server,
    const char* client_ip
);
```

**Parameters:**

- `server`: The server instance
- `client_ip`: The client IP address (currently unused, reserved for future expansion)

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter

**Note:**

- The current implementation resets the rate limit counter for the entire server
- The `client_ip` parameter is unused, reserved for future expansion

**Example:**

```c
uvhttp_server_reset_rate_limit_client(server, "127.0.0.1");
```

### Clear All Rate Limit State

```c
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server);
```

**Parameters:**

- `server`: The server instance

**Return value:**

- `UVHTTP_OK`: Success
- `UVHTTP_ERROR_INVALID_PARAM`: Invalid parameter

**Example:**

```c
uvhttp_server_clear_rate_limit_all(server);
```

## Usage Examples

### Basic Usage

```c
#include "uvhttp.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);

    // Enable rate limiting: at most 1000 requests per second
    uvhttp_server_enable_rate_limit(server, 1000, 1);

    // Add a whitelist entry
    uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");

    // Start the server
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_free(server);
    return 0;
}
```

### Adjusting Rate Limit Dynamically

```c
// Adjust rate limit parameters at runtime
void adjust_rate_limit(uvhttp_server_t* server, int new_max_requests) {
    // Disable first
    uvhttp_server_disable_rate_limit(server);

    // Re-enable
    uvhttp_server_enable_rate_limit(server, new_max_requests, 1);
}
```

## Rate Limit Response

When a request exceeds the rate limit, the server returns:

- **Status code**: 429 Too Many Requests
- **Content-Type**: text/plain
- **Retry-After**: 60 (suggests retrying after 60 seconds)
- **Response body**: "Too Many Requests"

## Design Notes

### Server-Level Rate Limiting

The current implementation uses server-level rate limiting, where all requests share the same rate limit counter. This design is suitable for:

- **DDoS protection**: Limits the total request volume the server receives
- **Resource protection**: Prevents the server from crashing due to overload
- **Simple and efficient**: No need to maintain per-client state

### Why Not Client-Level?

Client-level rate limiting requires:

1. Maintaining independent rate limit state for each client
2. Storing client state in a hash table
3. Periodically cleaning up expired client state
4. More complex memory management

For DDoS protection scenarios, server-level rate limiting is sufficient, and:

- **Better performance**: No hash table lookups
- **Less memory**: Only a single counter
- **Simpler implementation**: Easier to maintain code

### Future Expansion

If client-level rate limiting is needed, you could consider:

1. Using the `uthash` library to store client state
2. Implementing a sliding window algorithm
3. Adding client state expiration
4. Providing finer-grained rate limit control

## Error Codes

Rate-limit-related error codes:

```c
UVHTTP_ERROR_RATE_LIMIT_EXCEEDED = -550  // Rate limit exceeded
```

## Performance Considerations

- **Time complexity**: O(1) - fixed window algorithm
- **Space complexity**: O(1) - maintains only a single counter
- **Throughput**: ~819,000 checks/second
- **Average latency**: ~1.2 microseconds
- **Memory usage**: 12 bytes (rate limit context)
- **Overhead**: Minimal, only simple counting and comparison operations

### Performance Optimizations

1. **Zero memory allocation**: Rate limit state is embedded directly in the server structure, requiring no dynamic memory allocation
2. **Cache-friendly**: All hot data resides in the same cache line, reducing cache misses
3. **No pointer dereferencing**: Direct field access in the struct is faster than pointer access
4. **Compile-time optimization**: Implemented via conditional compilation; no runtime overhead when disabled

## Best Practices

1. **Configure rate limit parameters sensibly**

   - Set `max_requests` based on server performance
   - Set `window_seconds` based on business requirements
   - Avoid overly strict rate limiting that affects normal users

2. **Use the whitelist**

   - Add trusted IP addresses to the whitelist
   - Include monitoring systems, internal services, and similar
   - Avoid rate limiting affecting critical services

3. **Monitor rate limit status**

   - Periodically check `remaining` and `reset_time`
   - Log rate-limit-triggered events
   - Adjust parameters based on actual conditions

4. **Graceful degradation**
   - Return clear error messages when the rate limit is exceeded
   - Provide the `Retry-After` header
   - Consider implementing a cache layer to reduce server load

## Related Documentation

- [API Reference](../api/API_REFERENCE.md)
- [Architecture Design](../dev/ARCHITECTURE.md)
- Error code reference
