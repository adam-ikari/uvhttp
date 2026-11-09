# WebSocket 使用指南

## 概述

WebSocket 从 HTTP 升级而来：客户端发一个带 Upgrade 头的请求，服务器回升级成功的响应，之后同一条连接双向传消息。UVHTTP 把握手、帧解析和回调分发都放在库里，应用只需要填几个回调。

## WebSocket 工作原理

### 握手过程

1. 客户端发起 HTTP 请求，带上升级所需的头部：
   ```
   Upgrade: websocket
   Connection: Upgrade
   Sec-WebSocket-Key: <随机字符串>
   Sec-WebSocket-Version: 13
   ```

2. 服务器接受升级，回切换协议的状态码和计算出的 Sec-WebSocket-Accept：
   ```
   HTTP/1.1 101 Switching Protocols
   Upgrade: websocket
   Connection: Upgrade
   Sec-WebSocket-Accept: <计算后的字符串>
   ```

3. 握手完成后，双方在同一个连接上双向发送消息

## 基本使用

### 创建 WebSocket 服务器

注册处理器时把路径一起给出。on_message 负责收消息并回写，on_connect 的返回值决定接受还是拒绝。

```c
#include "uvhttp.h"

// WebSocket 连接建立回调
int on_connect(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket 连接建立\n");
    return 0;
}

// WebSocket 消息接收回调
int on_message(uvhttp_ws_connection_t* ws_conn, 
               const char* data, 
               size_t len, 
               int opcode) {
    (void)ws_conn;
    
    printf("收到消息: %.*s\n", (int)len, data);
    
    // 回显消息
    uvhttp_server_ws_send(ws_conn, data, len);
    
    return 0;
}

// WebSocket 连接关闭回调
int on_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    printf("WebSocket 连接关闭\n");
    return 0;
}

// WebSocket 错误回调
int on_error(uvhttp_ws_connection_t* ws_conn, 
             int error_code, 
             const char* error_msg) {
    (void)ws_conn;
    printf("WebSocket 错误: %d - %s\n", error_code, error_msg);
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_take_router(server, router);

    // 注册 WebSocket 处理器
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_connect,
        .on_message = on_message,
        .on_close = on_close,
        .on_error = on_error,
        .user_data = NULL
    };

    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);

    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    printf("WebSocket 服务器运行在 http://localhost:8080/ws\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // 清理资源
    uvhttp_server_free(server);
    return 0;
}
```

## WebSocket 消息类型

opcode 决定这一帧是什么内容：

- `0x0`: Continuation Frame（连续帧）
- `0x1`: Text Frame（文本帧）
- `0x2`: Binary Frame（二进制帧）
- `0x8`: Close Frame（关闭帧）
- `0x9`: Ping Frame（心跳帧）
- `0xA`: Pong Frame（心跳响应帧）

### 发送不同类型的消息

文本和二进制都走同一个发送接口，ping 和 close 各有专用接口。

```c
// 发送文本消息
const char* text = "Hello WebSocket";
uvhttp_server_ws_send(ws_conn, text, strlen(text));

// 发送二进制消息
const char* binary_data = "\x01\x02\x03\x04";
uvhttp_server_ws_send(ws_conn, binary_data, 4);

// 发送 Ping
uvhttp_ws_send_ping(NULL, ws_conn, (const uint8_t*)"ping", 4);

// 发送 Close
uvhttp_server_ws_close(ws_conn, 1000, "正常关闭");
```

## 应用层认证

认证放在应用层。`on_connect` 只拿到连接对象，看不到原始 HTTP 请求头，`Authorization` 这类信息在这里取不到。要按请求头判断，就在升级前于 HTTP 处理器或中间件里校验，再让 `on_connect` 决定收不收：

```c
int on_connect(uvhttp_ws_connection_t* ws_conn) {
    // on_connect 只接收 WebSocket 连接对象；原始 HTTP 请求头
    // （如 Authorization）在此不可访问。请在升级前的 HTTP 处理器/
    // 中间件中校验请求头，然后在此接受或拒绝连接。
    printf("WebSocket 连接建立\n");
    return 0;  // 接受连接
}
```

## 最佳实践

### 1. 连接管理

广播和清理都要用到连接表，自己维护一份。数组满了就拒绝新连接，别让它继续长：

```c
// 维护活跃连接列表
static uvhttp_ws_connection_t* g_connections[MAX_CONNECTIONS];
static int g_connection_count = 0;

int on_connect(uvhttp_ws_connection_t* ws_conn) {
    
    if (g_connection_count < MAX_CONNECTIONS) {
        g_connections[g_connection_count++] = ws_conn;
        printf("连接 %d 已建立\n", g_connection_count);
    } else {
        printf("连接数已达上限\n");
        return -1;
    }
    
    return 0;
}

int on_close(uvhttp_ws_connection_t* ws_conn) {
    
    // 从连接列表中移除
    for (int i = 0; i < g_connection_count; i++) {
        if (g_connections[i] == ws_conn) {
            // 移动最后一个元素到当前位置
            g_connections[i] = g_connections[--g_connection_count];
            break;
        }
    }
    
    printf("连接关闭，剩余 %d 个连接\n", g_connection_count);
    return 0;
}

// 广播消息到所有连接
void broadcast_message(const char* message, size_t len) {
    for (int i = 0; i < g_connection_count; i++) {
        uvhttp_server_ws_send(g_connections[i], message, len);
    }
}
```

### 2. 心跳检测

定时发 ping，两端都知道连接还活着，中间设备也不容易把空闲连接掐掉：

```c
// 定期发送 Ping
void heartbeat_timer_callback(uv_timer_t* handle) {
    const char* ping_msg = "ping";
    for (int i = 0; i < g_connection_count; i++) {
        uvhttp_ws_send_ping(NULL, g_connections[i], (const uint8_t*)ping_msg,
                            strlen(ping_msg));
    }
    
    // 重新设置定时器
    uv_timer_start(handle, heartbeat_timer_callback, 30000);
}

int main() {
    // ... 服务器初始化代码 ...
    
    // 创建心跳定时器
    uv_timer_t heartbeat_timer;
    uv_timer_init(loop, &heartbeat_timer);
    uv_timer_start(&heartbeat_timer, heartbeat_timer_callback, 30000);  // 30秒
    
    // ... 启动服务器 ...
}
```

### 3. 消息大小限制

在 on_message 里先看长度，超限就关连接并返回错误码，别把内存喂爆：

```c
#define MAX_MESSAGE_SIZE (1024 * 1024)  // 1MB

int on_message(uvhttp_ws_connection_t* ws_conn, 
               const char* data, 
               size_t len, 
               int opcode) {
    (void)ws_conn;
    (void)opcode;
    
    if (len > MAX_MESSAGE_SIZE) {
        printf("消息过大: %zu 字节\n", len);
        uvhttp_server_ws_close(ws_conn, 1009, "消息过大");
        return -1;
    }
    
    // 处理消息
    process_message(data, len);
    
    return 0;
}
```

### 4. 错误处理

关闭码分开处理：正常关闭记一笔，协议错误和数据类型不支持各自记日志，其余交给默认分支：

```c
int on_error(uvhttp_ws_connection_t* ws_conn, 
             int error_code, 
             const char* error_msg) {
    (void)ws_conn;
    
    printf("WebSocket 错误: %d - %s\n", error_code, error_msg);
    
    // 根据错误类型处理
    switch (error_code) {
        case 1000:  // 正常关闭
            printf("客户端正常关闭\n");
            break;
        case 1002:  // 协议错误
            printf("协议错误，关闭连接\n");
            break;
        case 1003:  // 不支持的数据类型
            printf("不支持的数据类型\n");
            break;
        default:
            printf("未知错误\n");
    }
    
    return 0;
}
```

## 客户端示例

两个最小客户端，用来验证服务器行为。浏览器里直接用 WebSocket 对象，Python 侧需要 websockets 库。

### JavaScript 客户端

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onopen = function() {
    console.log('WebSocket 连接已建立');
    ws.send('Hello Server');
};

ws.onmessage = function(event) {
    console.log('收到消息:', event.data);
};

ws.onerror = function(error) {
    console.error('WebSocket 错误:', error);
};

ws.onclose = function(event) {
    console.log('WebSocket 连接已关闭:', event.code, event.reason);
};
```

### Python 客户端

```python
import asyncio
import websockets

async def websocket_client():
    uri = "ws://localhost:8080/ws"
    async with websockets.connect(uri) as websocket:
        print("WebSocket 连接已建立")
        
        # 发送消息
        await websocket.send("Hello Server")
        
        # 接收消息
        async for message in websocket:
            print(f"收到消息: {message}")

asyncio.run(websocket_client())
```

### curl 测试

这个场景 curl 不好用，拿 websocat 连上去，敲一行发一行：

```bash
# 使用 websocat 测试 WebSocket 连接
websocat ws://localhost:8080/ws

# 发送消息
Hello Server

# 接收回显
Hello Server
```

## 常见问题

### Q: 如何限制连接数？

A: 在 `on_connect` 回调中维护一个连接列表，当达到上限时返回 -1 拒绝连接。

### Q: 如何实现房间/频道功能？

A: 在连接时让客户端发送加入房间的消息，服务器维护房间到连接的映射关系。

### Q: 如何处理大消息？

A: 在 `on_message` 回调中检查消息大小，超过限制时关闭连接。

### Q: 如何实现消息压缩？

A: 在发送前使用压缩算法（如 zlib）压缩数据，客户端接收后解压。

### Q: 如何处理连接断开重连？

A: 客户端实现自动重连逻辑，服务器端维护连接状态。

## 相关资源

- [WebSocket 协议规范](https://tools.ietf.org/html/rfc6455)
- [WebSocket API 参考](../../api/API_REFERENCE.md#websocket-700s)
- [WebSocket 示例代码](https://github.com/adam-ikari/uvhttp/tree/main/examples/05_websocket)
- 最佳实践