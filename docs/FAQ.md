# UVHTTP Frequently Asked Questions (FAQ)

This document answers common questions about using the UVHTTP library.

## Table of Contents

1. [Installation and Setup](#installation-and-setup)
2. [Server Management](#server-management)
3. [Request Processing](#request-processing)
4. [Response Handling](#response-handling)
5. [TLS/SSL Configuration](#tlsssl-configuration)
6. [WebSocket](#websocket)
7. [Performance Optimization](#performance-optimization)
8. [Common Issues](#common-issues)

---

## Installation and Setup

### Q1: What are the system requirements for UVHTTP?

**Answer:**
- **Operating System**: Linux, macOS, Windows
- **Compiler**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **Build System**: CMake 3.10+
- **Dependencies**: libuv (included), llhttp (included)

**Build Commands:**
```bash
mkdir build && cd build
cmake ..
make
```

### Q2: How do I enable optional features like WebSocket?

**Answer:**
Use CMake options when configuring the build:

```bash
# Enable WebSocket
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# Enable mimalloc allocator
cmake -DBUILD_WITH_MIMALLOC=ON ..

# Enable all features
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

### Q3: How do I switch between system allocator and mimalloc?

**Answer:**
Use the allocator type flag at compile time:

```bash
# System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc allocator
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

---

## Server Management

### Q4: How do I gracefully shutdown a server?

**Answer:**
```c
#include <signal.h>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
    uv_stop(loop);  // Stop the event loop
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    // ... setup server ...
    
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // Run until signal received
    uv_run(loop, UV_RUN_DEFAULT);
    
    // Cleanup
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    return 0;
}
```

### Q5: How do I change the server's address binding?

**Answer:**
```c
// Bind to specific IP
uvhttp_server_listen(server, "192.168.1.100", 8080);

// Bind to all interfaces
uvhttp_server_listen(server, "0.0.0.0", 8080);

// Bind to localhost only
uvhttp_server_listen(server, "127.0.0.1", 8080);
```

### Q6: How do I handle multiple servers on the same loop?

**Answer:**
```c
uv_loop_t* loop = uv_default_loop();

// Create multiple servers
uvhttp_server_t* server1 = uvhttp_server_new(loop);
uvhttp_server_t* server2 = uvhttp_server_new(loop);

// Configure each server
uvhttp_server_listen(server1, "0.0.0.0", 8080);
uvhttp_server_listen(server2, "0.0.0.0", 8081);

// All servers share the same event loop
uv_run(loop, UV_RUN_DEFAULT);
```

---

## Request Processing

### Q7: How do I access request headers?

**Answer:**
```c
// Get specific header
const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
const char* content_type = uvhttp_request_get_header(request, "Content-Type");

// Iterate over all headers
void header_callback(const char* name, const char* value, void* user_data) {
    printf("Header: %s: %s\n", name, value);
}
uvhttp_request_foreach_header(request, header_callback, NULL);
```

### Q8: How do I get the client's IP address?

**Answer:**
```c
// Get client IP address
const char* client_ip = uvhttp_request_get_client_ip(request);
```

### Q9: How do I handle request body?

**Answer:**
```c
// Get request body
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);

// Get Content-Length header
const char* content_length = uvhttp_request_get_header(request, "Content-Length");

// Process body...
```

### Q10: How do I handle query parameters?

**Answer:**
```c
// Get all query parameters
const char* query = uvhttp_request_get_query_string(request);

// Get specific query parameter
const char* search = uvhttp_request_get_query_param(request, "q");
const char* page = uvhttp_request_get_query_param(request, "page");
```

---

## Response Handling

### Q11: How do I set cookies in response?

**Answer:**
```c
uvhttp_response_set_header(response, "Set-Cookie", 
    "session=abc123; Path=/; HttpOnly; Secure; SameSite=Strict");
uvhttp_response_set_header(response, "Set-Cookie", 
    "theme=dark; Path=/; Max-Age=31536000");
```

### Q12: How do I enable CORS?

**Answer:**
```c
uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

// Handle preflight request
if (uvhttp_request_get_method(request) == UVHTTP_METHOD_OPTIONS) {
    uvhttp_response_set_status(response, 204);
    uvhttp_response_send(response);
    return 0;
}
```

### Q13: How do I send a file attachment?

**Answer:**
```c
uvhttp_response_set_header(response, "Content-Disposition", 
    "attachment; filename=\"example.txt\"");

const char* file_content = "File content here...";
uvhttp_response_set_body(response, file_content, strlen(file_content));
uvhttp_response_send(response);
```

### Q14: How do I handle large responses?

**Answer:**
```c
// For large responses, set the response body directly
// UVHTTP automatically handles large responses efficiently
const char* large_data = get_large_data();  // Your data source
size_t data_length = get_data_length();

uvhttp_response_set_status(response, 200);
uvhttp_response_set_body(response, large_data, data_length);
uvhttp_response_send(response);
```

---

## TLS/SSL Configuration

### Q15: How do I generate self-signed certificates for testing?

**Answer:**
```bash
# Generate CA private key
openssl genrsa -out ca.key 4096

# Generate CA certificate
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=MyCA"

# Generate server private key
openssl genrsa -out server.key 2048

# Generate CSR
openssl req -new -key server.key -out server.csr \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"

# Sign certificate with CA
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 365 \
  -extfile <(echo "subjectAltName=DNS:localhost,IP:127.0.0.1")
```

### Q16: How do I enable client certificate authentication?

**Answer:**
```c
// Enable client authentication
uvhttp_tls_context_enable_client_auth(tls_ctx, 1);

// Set verify depth
uvhttp_tls_context_set_verify_depth(tls_ctx, 3);

// Load client CA certificates
uvhttp_tls_context_load_ca_file(tls_ctx, "client_ca.crt");
```

### Q17: How do I configure cipher suites?

**Answer:**
```c
// Define cipher suites
static const int cipher_suites[] = {
    MBEDTLS_TLS_AES_256_GCM_SHA384,
    MBEDTLS_TLS_CHACHA20_POLY1305_SHA256,
    MBEDTLS_TLS_AES_128_GCM_SHA256,
    0  // Terminator
};

// Set cipher suites
uvhttp_tls_context_set_cipher_suites(tls_ctx, cipher_suites);
```

### Q18: How do I enable TLS 1.3?

**Answer:**
```c
// Enable TLS 1.3
uvhttp_tls_context_enable_tls13(tls_ctx, 1);

// Set minimum TLS version
// This is automatically handled when enabling TLS 1.3
```

---

## WebSocket

### Q19: How do I send messages to all connected WebSocket clients?

**Answer:**
```c
// Broadcast message to all clients on a specific path
uvhttp_server_ws_broadcast(server, "/ws", "Hello everyone", 15);
```

### Q20: How do I handle WebSocket ping/pong?

**Answer:**
```c
// Send ping (requires context)
uvhttp_ws_send_ping(context, ws_conn, (const uint8_t*)"heartbeat", 9);

// Handle pong in callback
int on_message(uvhttp_ws_connection_t* ws_conn, const char* data, 
               size_t len, int opcode) {
    if (opcode == UVHTTP_WS_OPCODE_PONG) {
        printf("Received pong: %s\n", data);
    }
    return 0;
}
```

---

## Performance Optimization

### Q21: How do I enable connection pooling?

**Answer:**
```c
// Enable Keep-Alive by setting configuration
uvhttp_config_t* config = uvhttp_config_create();
config->keepalive_timeout = 60;  // 60 seconds

uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_server_set_config(server, config);
```

### Q22: How do I optimize static file serving?

**Answer:**
```c
// Enable LRU cache for static files
// Automatically enabled when using uvhttp_static

// Pre-warm cache on startup
uvhttp_static_context_t* static_ctx = uvhttp_static_create("/var/www/static");
uvhttp_static_prewarm_directory(static_ctx, "/var/www/static", 100);

// Large files (>1MB) use sendfile automatically
// No additional configuration needed
```

### Q23: How do I monitor server performance?

**Answer:**
```c
// Get active connections
size_t active_connections = server->active_connections;

// Log statistics periodically
printf("Active connections: %zu\n", active_connections);
```

### Q24: How do I configure rate limiting?

**Answer:**
```c
// Enable rate limiting on server (1000 requests per second)
uvhttp_server_enable_rate_limit(server, 1000, 1);

// Add IP to whitelist (optional)
uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.100");
```

---

## Common Issues

### Q25: Server fails to start with "address already in use"

**Answer:**
```bash
# Find process using the port
lsof -i :8080
# or
netstat -tlnp | grep 8080

# Kill the process
kill -9 <PID>

# Or use a different port
uvhttp_server_listen(server, "0.0.0.0", 8081);
```

### Q26: Connection timeout errors

**Answer:**
```c
// Increase timeout values
uvhttp_config_t* config = uvhttp_config_create();
uvhttp_config_set_keepalive_timeout(config, 300);  // 5 minutes
uvhttp_config_set_request_timeout(config, 60);    // 1 minute

uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_server_set_config(server, config);
```

### Q27: Memory usage keeps increasing

**Answer:**
```bash
# Check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./your_server

# Or build with AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
make
./your_server
```

### Q28: High CPU usage

**Answer:**
```c
// Check for busy loops
// Ensure you're using UV_RUN_DEFAULT, not UV_RUN_NOWAIT in a tight loop

// Disable unnecessary logging
#define UVHTTP_FEATURE_LOGGING 0

// Reduce polling frequency
// Check connection timeouts and adjust as needed
```

### Q29: TLS handshake fails with "certificate verify failed"

**Answer:**
```c
// Disable certificate verification for testing (not recommended for production)
uvhttp_tls_context_enable_client_auth(tls_ctx, 0);

// For production, ensure:
// 1. Server certificate is valid
// 2. CA certificate is loaded
// 3. Certificate chain is complete
// 4. Certificate is not expired
// 5. Common Name (CN) matches the hostname
```

### Q30: How do I enable debug logging?

**Answer:**
```bash
# 1. Enable logging in include/uvhttp_features.h
#define UVHTTP_FEATURE_LOGGING 1

# 2. Build in Debug mode
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 3. Run with debug output
./your_server
```

---

## Best Practices

### Security
- Always use HTTPS in production
- Validate all user input
- Implement rate limiting
- Keep dependencies updated
- Use strong cipher suites

### Performance
- Enable Keep-Alive connections
- Use connection pooling
- Enable caching for static files
- Monitor resource usage
- Profile bottlenecks

### Reliability
- Implement graceful shutdown
- Add proper error handling
- Log important events
- Test under load
- Monitor server health

---

## Related Documentation

- [API Reference](api/API_REFERENCE.md)
- [Developer Guide](guide/DEVELOPER_GUIDE.md)
- [Tutorial](guide/TUTORIAL.md)

---

## Version

- **Document Version**: 1.0.0
- **Last Updated**: 2026-02-03
- **UVHTTP Version**: 2.2.0+