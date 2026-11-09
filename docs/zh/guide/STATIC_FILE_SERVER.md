---
title: UVHTTP 静态文件服务器指南
---
# UVHTTP 静态文件服务器指南

## 概述

静态文件服务由应用层实现。`uvhttp_static_handle_request()` 一次处理一个请求，路由、路径映射和上下文传递都留在应用侧。

## 设计原则

### 应用层实现

`uvhttp_static_handle_request()` 只认请求和响应，不认 URL 前缀。同一份处理逻辑既能挂在 /static/* 下，也能当全站回退，取决于你在路由表里怎么排。

### 实现方式

```c
// 1. 创建静态文件上下文
uvhttp_static_context_t* static_ctx;
uvhttp_static_create(&config, &static_ctx);

// 2. 创建应用层包装函数
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app_ctx = (app_context_t*)loop->data;
    return uvhttp_static_handle_request(app_ctx->static_ctx, request, response);
}

// 3. 添加路由（应用层控制）
uvhttp_router_add_route(router, "/static/*", static_file_handler);
uvhttp_router_add_route(router, "/*", static_file_handler);  // 回退路由
```

### 不提供内置静态路由的原因

- 库不认识你的 URL 布局，也就不需要为静态服务另立一套规则
- 哪些前缀进静态服务、哪些留给 API，由应用自己排
- 同一套处理函数可以复用到不同前缀和回退位置

## 核心特性

### 性能优化
- **LRU 缓存**: 内存缓存，减少磁盘 I/O
- **零拷贝**: sendfile 文件传输
- **连接复用**: 由 libuv 的事件循环承担
- **压缩支持**: 预留 gzip/deflate 接口

### 安全特性
- **路径安全验证**: 防止目录遍历
- **文件类型检查**: 白名单可配置
- **访问控制**: 基于路径的访问限制
- **资源限制**: 防止大文件 DoS

### 功能特性
- **自动 MIME 类型检测**: 覆盖常见文件类型
- **条件请求**: ETag 和 Last-Modified
- **目录列表**: 可配置的目录浏览
- **自定义头部**: 支持添加自定义 HTTP 头部
- **错误处理**: 错误页面和日志记录

## 快速开始

### 基础示例

完整示例在 `examples/04_static_files/static_file_server.c`。下面这段是骨架，把 root_directory 换成你的站点目录就能跑：

```c
#include "uvhttp.h"
#include "uvhttp_static.h"

static uvhttp_static_context_t* g_static_ctx = NULL;

static int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    return uvhttp_static_handle_request(g_static_ctx, request, response);
}

int main() {
    uvhttp_static_config_t config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB
        .cache_ttl = 3600                    // 1 小时
    };

    if (uvhttp_static_create(&config, &g_static_ctx) != UVHTTP_OK) {
        return 1;
    }

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);

    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_router_add_route(router, "/*", static_file_handler);
    uvhttp_server_take_router(server, router);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### 关键点
- 路由用 `uvhttp_router_add_route()` 添加，前缀写成通配符才能覆盖子路径
- 处理函数只做一层包装，真正的逻辑在 `uvhttp_static_handle_request()`
- 上下文从 `server->context` 或 `loop->data` 取，别用全局变量
- 通配符路由是多个静态路径共用的入口，它也会接住没匹配上的请求

### 高级配置

```c
uvhttp_static_config_t advanced_config = {
    .root_directory = "/var/www/html",
    .index_file = "index.html",
    .enable_directory_listing = 0,
    .max_cache_size = 100 * 1024 * 1024,  // 100MB
    .cache_ttl = 7200,                      // 2 小时
    .custom_headers = "X-Content-Type-Options: nosniff\r\n"
                     "X-Frame-Options: DENY\r\n"
                     "X-XSS-Protection: 1; mode=block",
    .max_file_size = 50 * 1024 * 1024     // 50MB
};
```

## 配置选项

### 基础配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `root_directory` | `const char*` | `"./public"` | 静态文件根目录 |
| `index_file` | `const char*` | `"index.html"` | 默认首页文件 |
| `enable_directory_listing` | `int` | `0` | 是否启用目录列表 |

### 缓存配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `max_cache_size` | `size_t` | 必填（须 > 0） | 最大缓存大小（字节） |
| `cache_ttl` | `int` | `0` | 缓存 TTL（秒），`0` 表示永不过期 |
| `max_cache_entries` | `int` | `0` | 最大条目数，`0` 表示使用内置默认值 |

### 安全配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `max_file_size` | `size_t` | `50*1024*1024` | 最大文件大小 |
| `enable_compression` | `int` | `0` | 是否启用压缩 |

### HTTP 配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `custom_headers` | `const char*` | `""` | 自定义 HTTP 头部 |

## API 参考

### 核心函数

#### `uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config, uvhttp_static_context_t** context)`

创建静态文件服务上下文。

**参数:**
- `config`: 静态文件配置
- `context`: 输出参数，返回创建的上下文

**返回值:**
- `UVHTTP_OK`: 成功
- 其他值: 错误码

#### `void uvhttp_static_free(uvhttp_static_context_t* ctx)`

释放静态文件服务上下文。

**参数:**
- `ctx`: 静态文件服务上下文

#### `uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx, uvhttp_request_t* request, uvhttp_response_t* response)`

处理静态文件请求。

**参数:**
- `ctx`: 静态文件服务上下文
- `request`: HTTP 请求对象
- `response`: HTTP 响应对象

**返回值:**
- `UVHTTP_OK`: 成功
- 其他值: 错误码

### 工具函数

#### `uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path, char* mime_type, size_t mime_type_size)`

根据文件路径获取 MIME 类型。

#### `int uvhttp_static_resolve_safe_path(const char* root_dir, const char* file_path, char* resolved_path, size_t buffer_size)`

检查文件路径是否安全（防止目录遍历）。

#### `uvhttp_result_t uvhttp_static_generate_etag(const char* file_path, time_t last_modified, size_t file_size, char* etag, size_t buffer_size)`

为文件生成 ETag 值。

## 最佳实践

### 1. 目录结构

```
project/
├── public/                 # 静态文件根目录
│   ├── css/               # 样式文件
│   ├── js/                # JavaScript 文件
│   ├── images/            # 图片文件
│   ├── fonts/             # 字体文件
│   └── docs/              # 文档文件
├── src/                   # 源代码
└── server.c               # 服务器主程序
```

### 2. 安全配置

```c
uvhttp_static_config_t secure_config = {
    .root_directory = "/var/www/html",
    .enable_directory_listing = 0,
};
```

## 文件类型控制

### MIME 类型映射

UVHTTP 使用内置的 MIME 类型映射表：

```c
static const uvhttp_mime_mapping_t default_mime_types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {NULL, NULL}
};
```

### 特点

- 扩展名到 MIME 类型的对应关系在编译期定死，运行时不做解析
- 映射由内置表 `uvhttp_mime_mapping_t` 驱动，应用不用再配一份扩展名列表
- 常见的 Web 类型都收在表里，覆盖大多数站点

### 扩展控制

1. **MIME 类型映射**：
   MIME 类型由内置映射表（`uvhttp_mime_mapping_t`）驱动。当前库未提供运行时新增映射的 API；如需自定义类型，请在源码的 MIME 映射表中添加条目后重新编译。

2. **使用请求处理器**：
   需要按路径放行时，先在包装函数里判断，不合规的请求直接回状态码：
```c
int custom_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_path(req);
    if (!is_allowed_file(path)) {
        uvhttp_response_set_status(res, 403);
        return uvhttp_response_send(res);
    }
    return uvhttp_response_send(res);
}
```

## 性能优化

### 零拷贝文件传输

文件传输走 sendfile，数据从页缓存直接进 socket，不经过用户态中转。

#### 文件大小分类策略

| 文件大小 | 传输方式 | 原因 |
|---------|---------|------|
| ≤ 64KB | 传统读取（open/read/close） | sendfile 在小文件上收益不足以抵消额外开销 |
| > 64KB | 分块 sendfile | 零拷贝，避免将整个文件读入内存 |

阈值由 `UVHTTP_SENDFILE_MIN_FILE_SIZE`（默认 64KB）控制。

#### 配置参数

```c
#define UVHTTP_SENDFILE_DEFAULT_TIMEOUT_MS   30000       // 30 秒超时
#define UVHTTP_SENDFILE_DEFAULT_MAX_RETRY    2           // 最大重试次数
#define UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE   (256 * 1024) // 256KB 分块
```

也可通过 `uvhttp_static_set_sendfile_config()` 在运行时覆盖，或在配置结构体中设置 `sendfile_timeout_ms` / `sendfile_max_retry` / `sendfile_chunk_size`（传 0 表示使用默认值）。

**配置结构体**：

```c
typedef struct uvhttp_static_config {
    size_t max_cache_size;
    size_t sendfile_chunk_size;
    size_t max_file_size;

    int cache_ttl;
    int max_cache_entries;
    int sendfile_timeout_ms;
    int sendfile_max_retry;
    int enable_directory_listing;

    char root_directory[UVHTTP_MAX_FILE_PATH_SIZE];
    char index_file[UVHTTP_MAX_PATH_SIZE];
    char custom_headers[UVHTTP_MAX_HEADER_VALUE_SIZE];
} uvhttp_static_config_t;
```

**参数选择依据**：

1. **sendfile_timeout_ms（默认 30 秒）**：
   - 超时后放弃这次传输，fd 不会一直开着
   - 默认值面向常见网络环境，慢链路可以调大
   - 可以经配置结构体覆盖

2. **sendfile_max_retry（默认 2 次）**：
   - 只对可恢复错误重试，次数用完就关 fd
   - 重试仅限 UV_EINTR、UV_EAGAIN
   - 无限重试只会把问题拖得更久

3. **sendfile_chunk_size（默认 256KB）**：
   - 分块小，系统调用次数多；分块大，单次占用内存多
   - 默认值取的是两者之间的折中
   - 对多数文件系统都适用

**动态配置 API**：

```c
uvhttp_error_t uvhttp_static_set_sendfile_config(
    uvhttp_static_context_t* ctx,
    int timeout_ms,
    int max_retry,
    size_t chunk_size
);
```

**配置示例**：

```c
uvhttp_static_config_t config;
memset(&config, 0, sizeof(config));
strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
config.max_cache_size = 100 * 1024 * 1024;  // 100MB
config.cache_ttl = 7200;  // 2 小时

config.sendfile_timeout_ms = 15000;  // 15 秒超时
config.sendfile_max_retry = 3;
config.sendfile_chunk_size = 128 * 1024;  // 128KB 分块

uvhttp_static_context_t* ctx = NULL;
uvhttp_static_create(&config, &ctx);
```

#### 超时检测机制

超时靠 libuv 定时器主动查，不等 sendfile 回调：

```c
uv_timer_init(loop, &ctx->timeout_timer);
uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout,
               SENDFILE_TIMEOUT_MS, 0);

static void on_sendfile_timeout(uv_timer_t* timer) {
    ctx->completed = 1;
    uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
}
```

**特点**：
- 网络完全阻塞时也能按时收尾
- 不依赖 sendfile 回调
- 到点后自己关 fd，不留悬挂资源

#### 错误处理和重试

```c
if (req->result < 0) {
    if (ctx->retry_count < SENDFILE_MAX_RETRY &&
        (req->result == UV_EINTR || req->result == UV_EAGAIN)) {
        ctx->retry_count++;
        uv_fs_sendfile(loop, &ctx->sendfile_req, ...);
        return;
    }

    uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
}
```

**重试策略**：
- 仅对可恢复错误重试（UV_EINTR、UV_EAGAIN）
- 最多重试 3 次
- 每次重试后重启超时定时器

### LRU 缓存系统

命中率不看不知道。把日志级别调到 DEBUG 再启动示例，命中、未命中和淘汰都会打出来：

```bash
export UVHTTP_LOG_LEVEL=DEBUG
./static_file_server
```

### 性能监控

运行时可以直接读缓存的几个计数：

```c
size_t total_memory, hit_count, miss_count;
uvhttp_static_get_cache_stats(ctx, &total_memory, &hit_count, &miss_count);

printf("缓存统计:\n");
printf("  内存使用: %zu bytes\n", total_memory);
printf("  命中次数: %zu\n", hit_count);
printf("  未命中次数: %zu\n", miss_count);
printf("  命中率: %.2f%%\n",
       (double)hit_count / (hit_count + miss_count) * 100);
```

## 缓存预热策略

预热 API 只负责把文件读进缓存。什么时候预热、预热哪些文件，由应用按访问模式决定。

### 可用的预热 API

```c
// 预热单个文件
uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,
                                            const char* file_path);

// 预热整个目录
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path, int max_files);

// 直接预热缓存（底层）
uvhttp_error_t uvhttp_lru_cache_prewarm(cache_manager_t* cache,
                                        const char* file_path, char* content,
                                        size_t content_length,
                                        const char* mime_type,
                                        time_t last_modified, const char* etag,
                                        int priority);
```

### 策略 1：按文件类型预热目录

启动时把 CSS、JS、图片这类共用资源读进缓存，首个访客就不用逐个等磁盘：

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

        // 检查扩展名是否为 Web 资源
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

// 用法
prewarm_web_assets(static_ctx, "./public/static");
```

### 策略 2：从 HTML 预加载关联文件

入口 HTML 引用的资源通常就是首屏要用的，扫一遍 href 和 src 就能找到：

```c
int preload_html_resources(uvhttp_static_context_t* ctx, const char* html_path) {
    // 读取 HTML 文件
    FILE* f = fopen(html_path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* html = malloc(size + 1);
    fread(html, 1, size, f);
    html[size] = '\0';
    fclose(f);

    // 提取资源路径（简化示例）
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

// 用法
preload_html_resources(static_ctx, "./public/index.html");
```

### 策略 3：按优先级预热

给文件设不同优先级，缓存满时先淘汰低分的：

```c
void prewarm_with_priority(uvhttp_static_context_t* ctx) {
    // 高优先级：核心 CSS 和 JS
    const char* high_priority_files[] = {
        "./public/css/main.css",
        "./public/js/app.js",
        "./public/js/vendor.js"
    };

    for (size_t i = 0; i < sizeof(high_priority_files) / sizeof(high_priority_files[0]); i++) {
        uvhttp_lru_cache_set_entry_priority(ctx->cache, high_priority_files[i], 100);
        uvhttp_static_prewarm_cache(ctx, high_priority_files[i]);
    }

    // 中优先级：图片
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

### 策略 4：服务器启动时渐进预热

分批读，启动阶段不会一次把大量文件拉进内存：

```c
void gradual_prewarm(uvhttp_static_context_t* ctx, const char* dir_path) {
    int batch_size = 10;
    int total_files = 0;
    int prewarmed = 0;

    // 第一批：核心文件
    prewarmed = uvhttp_static_prewarm_directory(ctx, dir_path, batch_size);
    printf("Prewarmed %d core files\n", prewarmed);

    // 第二批：附加文件
    prewarmed = uvhttp_static_prewarm_directory(ctx, dir_path, batch_size);
    printf("Prewarmed %d additional files\n", prewarmed);

    // 持续到缓存满或所有文件加载完成
    size_t total_memory;
    int entry_count;
    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory, &entry_count, NULL, NULL, NULL);
    printf("Cache stats: %zu bytes, %d entries\n", total_memory, entry_count);
}
```

### 策略 5：按需预热

首次请求时顺手把同目录的资源带进缓存，适合 URL 不固定的站点：

```c
int smart_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    app_context_t* app_ctx = (app_context_t*)req->client->loop->data;

    // 处理当前请求
    int result = uvhttp_static_handle_request(app_ctx->static_ctx, req, res);

    // 如果是 CSS 或 JS 文件，预加载关联文件
    if (result == UVHTTP_OK && req->path) {
        if (strstr(req->path, ".css") || strstr(req->path, ".js")) {
            char base_path[512];
            strncpy(base_path, req->path, sizeof(base_path));
            char* last_slash = strrchr(base_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                // 预热同目录下的其他文件
                uvhttp_static_prewarm_directory(app_ctx->static_ctx, base_path, 5);
            }
        }
    }

    return result;
}
```

### 性能监控

看命中率和条目数，判断策略有没有起作用：

```c
void print_cache_stats(uvhttp_static_context_t* ctx) {
    size_t total_memory;
    int entry_count;
    double hit_rate;

    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory, &entry_count, NULL, NULL, NULL);
    hit_rate = uvhttp_lru_cache_get_hit_rate(ctx->cache);

    printf("Cache Statistics:\n");
    printf("  Memory Usage: %zu bytes\n", total_memory);
    printf("  Entry Count: %d\n", entry_count);
    printf("  Hit Rate: %.2f%%\n", hit_rate * 100);
}
```

### 预热最佳实践

1. 先跑目录级预热，不够再上按 HTML 解析或按优先级那几套。
2. 缓存不是越大越好，盯着内存用量。
3. 常访问的文件给更高优先级，淘汰时先走冷数据。
4. 别什么都预热，只挑关键资源。
5. 用缓存统计找热点，再调整策略。

### 何时使用预热

- **生产部署**：接收流量之前先加载关键资源
- **零停机部署**：新实例接流量前预热
- **高流量事件**：为预期峰值准备缓存
- **性能调优**：按访问模式调整

### 何时不应使用预热

- **开发阶段**：文件一直在改，预热很快失效
- **小型应用**：缓存带来的收益抵不过维护成本
- **动态生成内容**：静态缓存无从下手
- **内存受限环境**：预热会挤占本就不够的内存

## 故障排除

### 常见问题

1. **404 错误**
   - 先核对请求路径和磁盘路径的映射规则
   - 确认文件在根目录下，注意大小写
   - 看进程有没有读权限

2. **403 错误**
   - 查权限位，确认进程读得了这个文件
   - 确认扩展名在白名单里
   - 看路径是不是被判为越界

3. **性能问题**
   - 缓存配大一点，看命中率有没有变化
   - 打开压缩，减少传输量
   - 测一下磁盘 I/O，排除硬件问题

4. **内存使用过高**
   - 调小缓存上限
   - 缩短 TTL，让冷数据尽快淘汰
   - 用缓存统计确认占用来源

### 调试技巧

调试日志是编译期开关，不是运行时配置：以 `-DENABLE_DEV_MODE=ON` 构建后使用 `UVHTTP_LOG_DEBUG`。

```c
UVHTTP_LOG_DEBUG("处理请求: %s %s",
                 uvhttp_request_get_method(request),
                 uvhttp_request_get_url(request));
```

## 示例项目

- `examples/04_static_files/static_file_server.c` - 基础静态文件服务器
- `examples/04_static_files/cache_test_server.c` - 缓存功能测试服务器

## 许可证

MIT License - 详见 LICENSE 文件
