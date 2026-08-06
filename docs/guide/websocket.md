# WebSocket Usage Guide

## Overview

UVHTTP provides full WebSocket support, allowing you to easily implement real-time bidirectional communication. The WebSocket protocol is built on top of the HTTP protocol, upgrading from HTTP to WebSocket through a handshake phase.

## How WebSocket Works

### The Handshake Process

1. The client makes an HTTP request containing special headers:
   ```
   Upgrade: websocket
   Connection: Upgrade
   Sec-WebSocket-Key: <random string>
   Sec-WebSocket-Version: 13
   ```

2. The server responds to the upgrade request:
   ```
   HTTP/1.1 101 Switching Protocols
   Upgrade: websocket
   Connection: Upgrade
   Sec-WebSocket-Accept: <computed string>
   ```

3. Once the connection is established, both parties can send messages in both directions

## Basic Usage

### Creating a WebSocket Server

```c
#include "uvhttp.h"

// WebSocket connection established callback
int on_connect(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    (void)user_data;
    printf("WebSocket connection established\n");
    return 0;
}

// WebSocket message received callback
int on_message(uvhttp_ws_connection_t* ws_conn, 
               const char* data, 
               size_t len, 
               int opcode, 
               void* user_data) {
    (void)ws_conn;
    (void)user_data;
    
    printf("Received message: %.*s\n", (int)len, data);
    
    // Echo the message
    uvhttp_server_ws_send(ws_conn, data, len);
    
    return 0;
}

// WebSocket connection closed callback
int on_close(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    (void)ws_conn;
    (void)user_data;
    printf("WebSocket connection closed\n");
    return 0;
}

// WebSocket error callback
int on_error(uvhttp_ws_connection_t* ws_conn, 
             int error_code, 
             const char* error_msg, 
             void* user_data) {
    (void)ws_conn;
    (void)user_data;
    printf("WebSocket error: %d - %s\n", error_code, error_msg);
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_set_router(server, router);

    // Register the WebSocket handler
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_connect,
        .on_message = on_message,
        .on_close = on_close,
        .on_error = on_error,
        .user_data = NULL
    };

    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);

    // Start the server
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    printf("WebSocket server running at http://localhost:8080/ws\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // Clean up resources
    uvhttp_server_free(server);
    return 0;
}
```

## WebSocket Message Types

WebSocket supports several message types (opcodes):

- `0x0`: Continuation Frame
- `0x1`: Text Frame
- `0x2`: Binary Frame
- `0x8`: Close Frame
- `0x9`: Ping Frame (heartbeat frame)
- `0xA`: Pong Frame (heartbeat response frame)

### Sending Different Types of Messages

```c
// Send a text message
const char* text = "Hello WebSocket";
uvhttp_server_ws_send(ws_conn, text, strlen(text));

// Send a binary message
const char* binary_data = "\x01\x02\x03\x04";
uvhttp_server_ws_send_binary(ws_conn, binary_data, 4);

// Send a Ping
uvhttp_server_ws_send_ping(ws_conn, "ping");

// Send a Close
uvhttp_server_ws_close(ws_conn, 1000, "Normal closure");
```

## Application-Level Authentication

Since authentication should be implemented at the application layer, you can authenticate during the WebSocket handshake:

```c
int on_connect(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    (void)user_data;
    
    // Get the HTTP request header
    const char* auth_header = uvhttp_ws_get_request_header(ws_conn, "Authorization");
    
    // Validate the Token
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        printf("Authentication failed: missing or invalid Token\n");
        return -1;  // Reject the connection
    }
    
    const char* token = auth_header + 7;
    if (!validate_token(token)) {
        printf("Authentication failed: invalid Token\n");
        return -1;
    }
    
    printf("Authentication successful\n");
    return 0;
}

bool validate_token(const char* token) {
    // Implement your token validation logic
    return strcmp(token, "my-secret-token") == 0;
}
```

## Best Practices

### 1. Connection Management

```c
// Maintain a list of active connections
static uvhttp_ws_connection_t* g_connections[MAX_CONNECTIONS];
static int g_connection_count = 0;

int on_connect(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    (void)user_data;
    
    if (g_connection_count < MAX_CONNECTIONS) {
        g_connections[g_connection_count++] = ws_conn;
        printf("Connection %d established\n", g_connection_count);
    } else {
        printf("Connection limit reached\n");
        return -1;
    }
    
    return 0;
}

int on_close(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    (void)user_data;
    
    // Remove from the connection list
    for (int i = 0; i < g_connection_count; i++) {
        if (g_connections[i] == ws_conn) {
            // Move the last element to the current position
            g_connections[i] = g_connections[--g_connection_count];
            break;
        }
    }
    
    printf("Connection closed, %d connections remaining\n", g_connection_count);
    return 0;
}

// Broadcast a message to all connections
void broadcast_message(const char* message, size_t len) {
    for (int i = 0; i < g_connection_count; i++) {
        uvhttp_server_ws_send(g_connections[i], message, len);
    }
}
```

### 2. Heartbeat Detection

```c
// Send a Ping periodically
void heartbeat_timer_callback(uv_timer_t* handle) {
    const char* ping_msg = "ping";
    for (int i = 0; i < g_connection_count; i++) {
        uvhttp_server_ws_send_ping(g_connections[i], ping_msg);
    }
    
    // Reset the timer
    uv_timer_start(handle, heartbeat_timer_callback, 30000);
}

int main() {
    // ... server initialization code ...
    
    // Create the heartbeat timer
    uv_timer_t heartbeat_timer;
    uv_timer_init(loop, &heartbeat_timer);
    uv_timer_start(&heartbeat_timer, heartbeat_timer_callback, 30000);  // 30 seconds
    
    // ... start the server ...
}
```

### 3. Message Size Limits

```c
#define MAX_MESSAGE_SIZE (1024 * 1024)  // 1MB

int on_message(uvhttp_ws_connection_t* ws_conn, 
               const char* data, 
               size_t len, 
               int opcode, 
               void* user_data) {
    (void)ws_conn;
    (void)opcode;
    (void)user_data;
    
    if (len > MAX_MESSAGE_SIZE) {
        printf("Message too large: %zu bytes\n", len);
        uvhttp_server_ws_close(ws_conn, 1009, "Message too large");
        return -1;
    }
    
    // Process the message
    process_message(data, len);
    
    return 0;
}
```

### 4. Error Handling

```c
int on_error(uvhttp_ws_connection_t* ws_conn, 
             int error_code, 
             const char* error_msg, 
             void* user_data) {
    (void)ws_conn;
    (void)user_data;
    
    printf("WebSocket error: %d - %s\n", error_code, error_msg);
    
    // Handle by error type
    switch (error_code) {
        case 1000:  // Normal closure
            printf("Client closed normally\n");
            break;
        case 1002:  // Protocol error
            printf("Protocol error, closing connection\n");
            break;
        case 1003:  // Unsupported data type
            printf("Unsupported data type\n");
            break;
        default:
            printf("Unknown error\n");
    }
    
    return 0;
}
```

## Client Examples

### JavaScript Client

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onopen = function() {
    console.log('WebSocket connection established');
    ws.send('Hello Server');
};

ws.onmessage = function(event) {
    console.log('Received message:', event.data);
};

ws.onerror = function(error) {
    console.error('WebSocket error:', error);
};

ws.onclose = function(event) {
    console.log('WebSocket connection closed:', event.code, event.reason);
};
```

### Python Client

```python
import asyncio
import websockets

async def websocket_client():
    uri = "ws://localhost:8080/ws"
    async with websockets.connect(uri) as websocket:
        print("WebSocket connection established")
        
        # Send a message
        await websocket.send("Hello Server")
        
        # Receive messages
        async for message in websocket:
            print(f"Received message: {message}")

asyncio.run(websocket_client())
```

### curl Testing

```bash
# Test the WebSocket connection with websocat
websocat ws://localhost:8080/ws

# Send a message
Hello Server

# Receive the echoed message
Hello Server
```

## FAQ

### Q: How do I limit the number of connections?

A: Maintain a connection list in the `on_connect` callback and return -1 to reject the connection when the limit is reached.

### Q: How do I implement room/channel functionality?

A: Have the client send a join-room message when connecting, and maintain a mapping from rooms to connections on the server.

### Q: How do I handle large messages?

A: Check the message size in the `on_message` callback and close the connection when it exceeds the limit.

### Q: How do I implement message compression?

A: Compress the data with a compression algorithm (such as zlib) before sending; the client decompresses it upon receipt.

### Q: How do I handle disconnection and reconnection?

A: Implement automatic reconnection logic on the client, and maintain connection state on the server.

## Related Resources

- [WebSocket Protocol Specification](https://tools.ietf.org/html/rfc6455)
- [WebSocket API Reference](../api/API_REFERENCE.md#websocket)
- [WebSocket Example Code](https://github.com/adam-ikari/uvhttp/tree/main/examples/05_websocket)
- best practices
