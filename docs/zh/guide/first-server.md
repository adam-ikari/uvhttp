# 第一个服务器

创建第一个 UVHTTP 服务器。

## Hello World 服务器

简单的 HTTP 服务器，响应 "Hello, World!"。

### 代码

创建 `hello.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");

    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建服务器失败: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建路由器失败: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/", hello_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("服务器运行在 http://localhost:8080\n");
    printf("按 Ctrl+C 停止服务器\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译

```bash
# 方法 1: 使用 CMake
mkdir build && cd build
cmake ..
make hello_world

# 方法 2: 直接编译
gcc -o hello hello.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

# 运行
./hello_world
```

### 测试

在浏览器中访问 `http://localhost:8080`，或：

```bash
curl http://localhost:8080/
```

## JSON API 服务器

返回 JSON 的 API 服务器。

### 代码

创建 `json_api.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json; charset=utf-8");

    const char* json_body = "{"
        "\"status\": \"success\","
        "\"message\": \"Hello from UVHTTP!\","
        "\"version\": \"1.0.0\""
    "}";

    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);

    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建服务器失败: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建路由器失败: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/api", api_handler);
    uvhttp_router_add_route(router, "/api/status", api_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("JSON API 服务器运行在 http://localhost:8080\n");
    printf("  - http://localhost:8080/api\n");
    printf("  - http://localhost:8080/api/status\n");
    printf("按 Ctrl+C 停止服务器\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译和运行

```bash
gcc -o json_api json_api.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./json_api
```

### 测试

```bash
curl http://localhost:8080/api
```

响应：
```json
{
  "status": "success",
  "message": "Hello from UVHTTP!",
  "version": "1.0.0"
}
```

## 路径参数服务器

处理路径参数的服务器。

### 代码

创建 `path_params.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int user_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    const char* username = path + 7;  // 跳过 "/users/"
    const char* format = uvhttp_request_get_query_param(request, "format");

    printf("请求用户: %s\n", username);

    uvhttp_response_set_status(response, 200);

    if (format && strcmp(format, "json") == 0) {
        char json_body[256];
        snprintf(json_body, sizeof(json_body),
                 "{\"username\":\"%s\",\"status\":\"active\"}", username);

        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, json_body, strlen(json_body));
    } else {
        char text_body[256];
        snprintf(text_body, sizeof(text_body),
                 "User: %s\nStatus: Active", username);

        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, text_body, strlen(text_body));
    }

    uvhttp_response_send(response);
    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t r = uvhttp_server_new(loop, &g_server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建服务器失败: %s\n", uvhttp_error_string(r));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    r = uvhttp_router_new(&router);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建路由器失败: %s\n", uvhttp_error_string(r));
        uvhttp_server_free(g_server);
        return 1;
    }

    uvhttp_server_set_router(g_server, router);
    uvhttp_router_add_route(router, "/users/*", user_handler);

    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }

    printf("路径参数服务器运行在 http://localhost:8080\n");
    printf("  - http://localhost:8080/users/alice\n");
    printf("  - http://localhost:8080/users/bob?format=json\n");
    printf("按 Ctrl+C 停止服务器\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译和运行

```bash
gcc -o path_params path_params.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./path_params
```

### 测试

```bash
# 文本格式
curl http://localhost:8080/users/alice

# JSON 格式
curl http://localhost:8080/users/bob?format=json
```

## 下一步

- [路由系统](./routing.md) - 更复杂的路由配置
- [请求处理](./requests.md) - HTTP 请求处理
- [响应处理](./responses.md) - 发送不同类型的响应
- [完整教程](../TUTORIAL.md) - 从基础到高级

## 常见问题

### Q: 如何监听其他端口？

修改 `uvhttp_server_listen` 的端口参数：
```c
uvhttp_server_listen(g_server, "0.0.0.0", 9000);  // 使用 9000 端口
```

### Q: 如何监听特定 IP？

修改 `uvhttp_server_listen` 的主机参数：
```c
uvhttp_server_listen(g_server, "127.0.0.1", 8080);  // 只监听本地
```

### Q: 如何处理 POST 请求？

在处理器中检查请求方法：
```c
const char* method = uvhttp_request_get_method(request);
if (strcmp(method, "POST") == 0) {
    // 处理 POST 请求
}
```

### Q: 如何获取请求体？

使用 `uvhttp_request_get_body`：
```c
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);
```
