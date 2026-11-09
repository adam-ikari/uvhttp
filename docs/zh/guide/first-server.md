# 你的第一个服务器

这一页从空文件写起，做三个能跑的小服务器：纯文本、JSON、带路径参数的。三个都用同一个端口，跑完一个再起下一个；每段都能用 curl 验证，适合先确认编译链没问题，再去看路由和中间件。

## Hello World 服务器

先只注册一个处理器，固定返回一个字符串。这一步跑通，说明头文件、库和链接参数都对上了。

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
    printf("\nReceived signal %d, shutting down server...\n", sig);
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

    /* 原子构造：一次调用完成 server + 路由 + 监听，无中间态、无需回滚。
     *
     * （v3.0.0 之前这里要 5 步，每个失败分支都得自己写清理。而最顺手的那种清理
     * 恰恰是错的：take_router 之后 router 已归 server 所有，再对它调一次
     * uvhttp_router_free 就是 double free。）*/
    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, hello_handler},
    };

    uvhttp_error_t result =
        uvhttp_server_listen_routes(loop, routes, 1, "0.0.0.0", 8080, &g_server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译

先构建库，再编这个文件。CMake 那条路会把依赖一起带上；直接用 gcc 时，头文件和库目录都得自己指，-I 和 -L 后面的路径按你的检出位置写，别照抄。两条编译命令的产物名字不同，运行时命令要和编译方式对应，拿错了就是“找不到文件”。

```bash
# 方法 1: 使用 CMake
make build
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

服务器启动后会打印监听地址。浏览器打开 `http://localhost:8080`，或者用 curl：

```bash
curl http://localhost:8080/
```

返回的就是处理器里写死的那行 Hello, World!。如果 curl 报连接被拒，先看服务器进程还在不在，再看端口有没有被别的程序占着。

## JSON API 服务器

把 Content-Type 换成 JSON，body 换成拼好的 JSON 字符串。两个路由挂同一个处理器，访问哪个都返回同一份内容。

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
    printf("\nReceived signal %d, shutting down server...\n", sig);
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

    const uvhttp_route_t routes[] = {
        {"/api",        UVHTTP_ANY, api_handler},
        {"/api/status", UVHTTP_ANY, api_handler},
    };

    uvhttp_error_t result =
        uvhttp_server_listen_routes(loop, routes, 2, "0.0.0.0", 8080, &g_server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("JSON API server running at http://localhost:8080\n");
    printf("  - http://localhost:8080/api\n");
    printf("  - http://localhost:8080/api/status\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译和运行

命令和前面那条一样，只改源文件名。编出来的程序直接跑，终端会列出两个可访问的路径。

```bash
gcc -o json_api json_api.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./json_api
```

### 测试

两个路由返回的内容相同，响应体是三个字段：

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

路由里写 :id，处理器自己从路径里取用户名；query 里的 format 决定返回纯文本还是 JSON。取名字的办法很直接：跳过路径开头的固定前缀。前缀一改，跳过的长度也得跟着改，这是这段代码最容易出错的地方。

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
    printf("\nReceived signal %d, shutting down server...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int user_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    const char* username = path + 7;  // Skip "/users/"
    const char* format = uvhttp_request_get_query_param(request, "format");

    printf("Requested user: %s\n", username);

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

    const uvhttp_route_t routes[] = {
        {"/users/:id", UVHTTP_ANY, user_handler},
    };

    uvhttp_error_t result =
        uvhttp_server_listen_routes(loop, routes, 1, "0.0.0.0", 8080, &g_server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Path parameter server running at http://localhost:8080\n");
    printf("  - http://localhost:8080/users/alice\n");
    printf("  - http://localhost:8080/users/bob?format=json\n");
    printf("Press Ctrl+C to stop the server\n");

    uv_run(loop, UV_RUN_DEFAULT);

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }

    return 0;
}
```

### 编译和运行

同样只换源文件名。启动后它会把两个示例 URL 打印出来，照着敲就行。

```bash
gcc -o path_params path_params.c \
    -I../include \
    -L./dist/lib \
    -luvhttp -luv -lpthread -lm -ldl

./path_params
```

### 测试

文本分支返回用户名和状态两行，JSON 分支返回一个对象；format 不传或传了别的值，都走文本分支。

```bash
# 文本格式
curl http://localhost:8080/users/alice

# JSON 格式
curl http://localhost:8080/users/bob?format=json
```

## 下一步

- 路由系统：前缀匹配、路径参数和方法限制
- 请求处理：读请求头、查询参数和请求体
- 响应处理：状态码、头部和不同 body 的写法
- [完整教程](./TUTORIAL.md)：从基础到高级

## 常见问题

### Q: 如何监听其他端口？

端口是 `uvhttp_server_listen` 的第三个参数，改这里就行。改完记得同步改客户端的地址。

```c
uvhttp_server_listen(g_server, "0.0.0.0", 9000);  // 使用 9000 端口
```

### Q: 如何监听特定 IP？

第二个参数是绑定地址，改的是 `uvhttp_server_listen` 的主机参数。绑回环地址只有本机能连，绑所有网卡则局域网可见；线上机器一般不直接暴露所有网卡。

```c
uvhttp_server_listen(g_server, "127.0.0.1", 8080);  // 只监听本地
```

### Q: 如何处理 POST 请求？

先取请求方法，再和字符串比较。非 POST 的分支也要有返回值，别让处理器漏掉。

```c
const char* method = uvhttp_request_get_method(request);
if (strcmp(method, "POST") == 0) {
    // 处理 POST 请求
}
```

### Q: 如何获取请求体？

请求体和长度一起取。`uvhttp_request_get_body` 给出指针，长度另外调一次接口拿，二进制 body 或中间带空字符的内容才不会截断。

```c
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);
```
