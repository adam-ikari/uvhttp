# WebSocket

## 概述

UVHTTP 提供完整的 WebSocket 支持，包括连接管理、消息收发、认证等功能。

## 启用 WebSocket

### 编译配置

```bash
cmake -DBUILD_WITH_WEBSOCKET=ON ..
make -j$(nproc)
```

## 基本使用

### 创建 WebSocket 服务器

```c
#include <uvhttp.h>
#include <uvhttp_websocket.h>

void on_message(uvhttp_ws_connection_t* conn, const char* message, size_t len) {
    printf("Received: %s\n", message);

    // 回复消息
    uvhttp_ws_send(conn, message, len);
}

void on_connect(uvhttp_ws_connection_t* conn) {
    printf("Client connected\n");
}

void on_disconnect(uvhttp_ws_connection_t* conn) {
    printf("Client disconnected\n");
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建 WebSocket 中间件
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_new();

    // 设置回调
    uvhttp_ws_middleware_set_callbacks(ws_middleware, on_connect, on_message, on_disconnect);

    // 添加到服务器
    uvhttp_server_add_middleware(server, ws_middleware);

    // 添加 WebSocket 路由
    uvhttp_router_add_route(server->router, "/ws", uvhttp_ws_handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 消息类型

### 文本消息

```c
void on_message(uvhttp_ws_connection_t* conn, const char* message, size_t len) {
    // 处理文本消息
    printf("Text: %s\n", message);
}
```

### 二进制消息

```c
void on_binary(uvhttp_ws_connection_t* conn, const void* data, size_t len) {
    // 处理二进制消息
    printf("Binary: %zu bytes\n", len);
}
```

### 发送消息

```c
// 发送文本消息
uvhttp_ws_send_text(conn, "Hello, WebSocket!");

// 发送二进制消息
char binary_data[1024];
uvhttp_ws_send_binary(conn, binary_data, sizeof(binary_data));

// 广播消息（发送给所有连接）
uvhttp_ws_broadcast(ws_middleware, "Broadcast message");
```

## 连接管理

### 获取连接信息

```c
void on_connect(uvhttp_ws_connection_t* conn) {
    // 获取连接 ID
    uint64_t conn_id = uvhttp_ws_get_connection_id(conn);

    // 获取客户端地址
    const char* client_ip = uvhttp_ws_get_client_ip(conn);
    int client_port = uvhttp_ws_get_client_port(conn);

    printf("Connected: %s:%d (ID: %lu)\n", client_ip, client_port, conn_id);
}
```

### 断开连接

```c
// 主动断开连接
uvhttp_ws_close(conn);

// 发送关闭帧
uvhttp_ws_close_with_status(conn, 1000, "Normal closure");
```

### 连接状态

```c
// 检查连接状态
if (uvhttp_ws_is_connected(conn)) {
    // 连接活跃
}

// 获取连接时间
time_t connect_time = uvhttp_ws_get_connect_time(conn);
```

## 认证

### 握手认证

```c
int on_authenticate(uvhttp_ws_connection_t* conn, const char* token) {
    // 验证 token
    if (strcmp(token, "secret-token") == 0) {
        return 1;  // 认证成功
    }
    return 0;  // 认证失败
}

int main() {
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_new();

    // 设置认证回调
    uvhttp_ws_middleware_set_authenticate(ws_middleware, on_authenticate);

    // ...
}
```

### 查询参数认证

```c
int on_authenticate(uvhttp_ws_connection_t* conn, const char* token) {
    // 从查询参数获取 token
    const char* query = uvhttp_ws_get_query_string(conn);

    // 解析查询参数
    char token_buf[256];
    if (get_query_param(query, "token", token_buf, sizeof(token_buf))) {
        return validate_token(token_buf);
    }

    return 0;
}
```

## 心跳检测

### 启用心跳

```c
int main() {
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_new();

    // 启用心跳检测
    uvhttp_ws_middleware_enable_heartbeat(ws_middleware, 30);  // 30秒

    // 设置心跳超时回调
    uvhttp_ws_middleware_set_heartbeat_timeout(ws_middleware, on_timeout);

    // ...
}

void on_timeout(uvhttp_ws_connection_t* conn) {
    printf("Connection timeout, closing...\n");
    uvhttp_ws_close(conn);
}
```

### 心跳消息

```c
void on_heartbeat(uvhttp_ws_connection_t* conn) {
    // 发送心跳响应
    uvhttp_ws_send_pong(conn);
}
```

## 房间和频道

### 创建房间

```c
// 创建房间
uvhttp_ws_room_t* room = uvhttp_ws_room_create("chat-room");

// 添加连接到房间
uvhttp_ws_room_add(room, conn);

// 从房间移除连接
uvhttp_ws_room_remove(room, conn);

// 向房间广播消息
uvhttp_ws_room_broadcast(room, "Room message");
```

### 房间管理

```c
// 获取房间中的连接数
size_t count = uvhttp_ws_room_count(room);

// 获取房间中的所有连接
uvhttp_ws_connection_t** connections = uvhttp_ws_room_get_connections(room, &count);

// 销毁房间
uvhttp_ws_room_destroy(room);
```

## 错误处理

### 错误码

```c
void on_error(uvhttp_ws_connection_t* conn, int error_code, const char* message) {
    printf("WebSocket error: %d - %s\n", error_code, message);

    // 常见错误码：
    // 1000 - 正常关闭
    // 1001 - 端点离开
    // 1002 - 协议错误
    // 1003 - 不支持的数据类型
    // 1006 - 连接异常关闭
    // 1007 - 数据类型不一致
    // 1008 - 违反策略
    // 1009 - 消息过大
    // 1010 - 缺少扩展
    // 1011 - 内部错误
}
```

### 错误恢复

```c
void on_error(uvhttp_ws_connection_t* conn, int error_code, const char* message) {
    if (error_code == 1006) {
        // 连接异常关闭，尝试重连
        printf("Connection lost, attempting to reconnect...\n");
        // 重连逻辑
    }
}
```

## 性能优化

### 消息压缩

```c
int main() {
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_new();

    // 启用消息压缩
    uvhttp_ws_middleware_enable_compression(ws_middleware);

    // ...
}
```

### 消息队列

```c
// 设置消息队列大小
uvhttp_ws_middleware_set_queue_size(ws_middleware, 1000);

// 检查队列状态
size_t queue_size = uvhttp_ws_get_queue_size(conn);
```

### 连接池

```c
// 设置最大连接数
uvhttp_ws_middleware_set_max_connections(ws_middleware, 1000);

// 获取当前连接数
size_t current_connections = uvhttp_ws_middleware_get_connection_count(ws_middleware);
```

## 示例项目

### 聊天服务器

```c
#include <uvhttp.h>
#include <uvhttp_websocket.h>

uvhttp_ws_room_t* chat_room;

void on_connect(uvhttp_ws_connection_t* conn) {
    printf("User connected\n");
    uvhttp_ws_room_add(chat_room, conn);

    // 广播用户加入消息
    char message[256];
    snprintf(message, sizeof(message), "User %lu joined", uvhttp_ws_get_connection_id(conn));
    uvhttp_ws_room_broadcast(chat_room, message);
}

void on_message(uvhttp_ws_connection_t* conn, const char* message, size_t len) {
    printf("Message: %s\n", message);

    // 广播消息到房间
    char broadcast[512];
    snprintf(broadcast, sizeof(broadcast), "User %lu: %s",
             uvhttp_ws_get_connection_id(conn), message);
    uvhttp_ws_room_broadcast(chat_room, broadcast);
}

void on_disconnect(uvhttp_ws_connection_t* conn) {
    printf("User disconnected\n");
    uvhttp_ws_room_remove(chat_room, conn);

    // 广播用户离开消息
    char message[256];
    snprintf(message, sizeof(message), "User %lu left", uvhttp_ws_get_connection_id(conn));
    uvhttp_ws_room_broadcast(chat_room, message);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    // 创建聊天房间
    chat_room = uvhttp_ws_room_create("chat");

    // 创建 WebSocket 中间件
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_new();
    uvhttp_ws_middleware_set_callbacks(ws_middleware, on_connect, on_message, on_disconnect);
    uvhttp_ws_middleware_enable_heartbeat(ws_middleware, 30);

    uvhttp_server_add_middleware(server, ws_middleware);
    uvhttp_router_add_route(router, "/ws", uvhttp_ws_handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 客户端示例

### JavaScript 客户端

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onopen = function() {
    console.log('Connected');
    ws.send('Hello, Server!');
};

ws.onmessage = function(event) {
    console.log('Received:', event.data);
};

ws.onclose = function(event) {
    console.log('Disconnected:', event.code, event.reason);
};

ws.onerror = function(error) {
    console.log('Error:', error);
};
```

### Python 客户端

```python
import websocket

def on_message(ws, message):
    print(f"Received: {message}")

def on_error(ws, error):
    print(f"Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("Disconnected")

def on_open(ws):
    print("Connected")
    ws.send("Hello, Server!")

if __name__ == "__main__":
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://localhost:8080/ws",
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)

    ws.run_forever()
```

## 更多资源

- [API 文档](/api/websocket)
- [WebSocket 认证](https://github.com/adam-ikari/uvhttp/blob/main/docs/WEBSOCKET_AUTH.md)
- [最佳实践](/guide/best-practices)