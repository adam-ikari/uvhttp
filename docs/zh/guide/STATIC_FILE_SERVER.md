---
title: UVHTTP 静态文件服务器指南
---
# UVHTTP 静态文件服务器指南

## 概述

静态文件服务由应用层实现，框架提供 `uvhttp_static_handle_request()` 处理单个请求。

## 设计原则

### 应用层实现

- **框架核心**: `uvhttp_static_handle_request()` 处理单个静态文件请求
- **应用层**: 负责路由配置、路径映射、上下文传递
- **灵活性**: 应用层控制静态文件服务的路由策略

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

- 避免框架臃肿
- 保持应用层的灵活性和控制力
- 符合极简工程原则

## 核心特性

### 性能优化
- **LRU 缓存**: 内存缓存，减少磁盘 I/O
- **零拷贝**: sendfile 文件传输
- **连接复用**: 基于 libuv 的事件驱动架构
- **压缩支持**: 预留 gzip/deflate 接口

### 安全特性
- **路径安全验证**: 防止目录遍历
- **文件类型检查**: 可配置的文件类型白名单
- **访问控制**: 基于路径的访问限制
- **资源限制**: 防止大文件 DoS

### 功能特性
- **自动 MIME 类型检测**: 支持常见文件类型
- **条件请求**: ETag 和 Last-Modified
- **目录列表**: 可配置的目录浏览
- **自定义头部**: 支持添加自定义 HTTP 头部
- **错误处理**: 错误页面和日志记录

## 快速开始

### 基础示例

见 `examples/04_static_files/static_file_server.c` 完整示例。

### 关键点
- 使用 `uvhttp_router_add_route()` 添加静态文件路由
- 创建包装函数调用 `uvhttp_static_handle_request()`
- 通过 `server->context` 或 `loop->data` 传递应用上下文
- 使用通配符路由处理多个静态文件路径

## 最佳实践

### 关键点

```c
#include "uvhttp.h"
#include "uvhttp_static.h"

int main() {
    uvhttp_static_config_t config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB
        .cache_ttl = 3600                      // 1 小时
    };

    uvhttp_static_context_t* ctx = NULL;
    uvhttp_static_create(&config, &ctx);

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);

    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_router_add_route(router, "/*", static_file_handler);
    uvhttp_server_set_router(server, router);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### 高级配置

```c
uvhttp_static_config_t advanced_config = {
    .root_directory = "/var/www/html",
    .index_file = "index.html",
    .enable_directory_listing = 0,
    .enable_etag = 1,
    .enable_last_modified = 1,
    .max_cache_size = 100 * 1024 * 1024,  // 100MB
    .cache_ttl = 7200,                      // 2 小时
    .custom_headers = "X-Content-Type-Options: nosniff\r\n"
                     "X-Frame-Options: DENY\r\n"
                     "X-XSS-Protection: 1; mode=block",
    .max_file_size = 50 * 1024 * 1024,     // 50MB
    .enable_compression = 1
};
```

## 配置选项

### 基础配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `root_directory` | `const char*` | `"./public"` | 静态文件根目录 |
| `index_file` | `const char*` | `"index.html"` | 默认首页文件 |
| `enable_directory_listing` | `int` | `1` | 是否启用目录列表 |
| `enable_etag` | `int` | `1` | 是否启用 ETag |
| `enable_last_modified` | `int` | `1` | 是否启用 Last-Modified |

### 缓存配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `max_cache_size` | `size_t` | `10*1024*1024` | 最大缓存大小（字节） |
| `cache_ttl` | `int` | `3600` | 缓存 TTL（秒） |

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

#### `uvhttp_static_context_t* uvhttp_static_create(const uvhttp_static_config_t* config)`

创建静态文件服务上下文。

**参数:**
- `config`: 静态文件配置

**返回值:**
- 成功: 上下文指针
- 失败: `NULL`

#### `void uvhttp_static_free(uvhttp_static_context_t* ctx)`

释放静态文件服务上下文。

**参数:**
- `ctx`: 静态文件服务上下文

#### `int uvhttp_static_handle_request(uvhttp_static_context_t* ctx, uvhttp_request_t* request, uvhttp_response_t* response)`

处理静态文件请求。

**参数:**
- `ctx`: 静态文件服务上下文
- `request`: HTTP 请求对象
- `response`: HTTP 响应对象

**返回值:**
- `0`: 成功
- 非 `0`: 错误码

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

- 无需配置扩展名列表
- 编译时确定，无运行时解析开销
- 基于 MIME 类型而非简单扩展名匹配
- MIME 类型由内置映射表（`uvhttp_mime_mapping_t`）驱动，无需配置扩展名列表

### 扩展控制

1. **MIME 类型映射**：
   MIME 类型由内置映射表（`uvhttp_mime_mapping_t`）驱动。当前库未提供运行时新增映射的 API；如需自定义类型，请在源码的 MIME 映射表中添加条目后重新编译。

2. **使用请求处理器**：
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

UVHTTP 使用 sendfile 系统调用实现零拷贝文件传输。

#### 文件大小分类策略

| 文件大小 | 传输方式 | 原因 |
|---------|---------|------|
| < 4KB | 传统方式（open/read/close） | 减少系统调用开销 |
| 4KB - 10MB | 分块 sendfile | 平衡性能和可靠性 |
| > 10MB | 分块 sendfile | 避免长时间阻塞 |

#### 配置参数

```c
#define SENDFILE_DEFAULT_TIMEOUT_MS  10000  // 10 秒超时
#define SENDFILE_DEFAULT_MAX_RETRY    2      // 最大重试次数
#define SENDFILE_DEFAULT_CHUNK_SIZE   (64 * 1024)  // 64KB 分块
```

**配置结构体**：

```c
typedef struct uvhttp_static_config {
    int enable_etag;
    int enable_last_modified;
    int enable_directory_listing;

    int enable_sendfile;
    int sendfile_timeout_ms;
    int sendfile_max_retry;
    size_t sendfile_chunk_size;

    size_t max_cache_size;
    int cache_ttl;
    int max_cache_entries;

    char root_directory[UVHTTP_MAX_FILE_PATH_SIZE];
    char index_file[UVHTTP_MAX_PATH_SIZE];
    char custom_headers[UVHTTP_MAX_HEADER_VALUE_SIZE];
} uvhttp_static_config_t;
```

**参数选择依据**：

1. **sendfile_timeout_ms（默认 10 秒）**：
   - 更快失败，减少资源占用
   - 适合大多数网络环境
   - 可通过配置文件自定义

2. **sendfile_max_retry（默认 2 次）**：
   - 减少重试以降低延迟
   - 仅对可恢复错误（UV_EINTR、UV_EAGAIN）重试
   - 避免无限重试导致资源浪费

3. **sendfile_chunk_size（默认 64KB）**：
   - 优化小文件传输，减少延迟
   - 平衡系统调用次数和内存使用
   - 适合大多数文件系统

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
config.enable_etag = 1;
config.enable_last_modified = 1;
config.max_cache_size = 100 * 1024 * 1024;  // 100MB
config.cache_ttl = 7200;  // 2 小时

config.enable_sendfile = 1;
config.sendfile_timeout_ms = 15000;  // 15 秒超时
config.sendfile_max_retry = 3;
config.sendfile_chunk_size = 128 * 1024;  // 128KB 分块

uvhttp_static_context_t* ctx = NULL;
uvhttp_static_create(&config, &ctx);
```

#### 超时检测机制

使用 libuv 定时器实现主动超时检测：

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
- 主动检测，即使网络完全阻塞也能及时响应
- 不依赖 sendfile 回调
- 自动清理资源，避免泄漏

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

```bash
export UVHTTP_LOG_LEVEL=DEBUG
./static_file_server
```

### 性能监控

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

UVHTTP 提供缓存预热 API，应用层可据此为自己的使用场景实现策略。框架提供基础设施，策略由应用决定。

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

从目录预加载常见的 Web 资源类型：

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

解析 HTML 提取并预加载引用的资源：

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

为不同文件类型设置不同优先级：

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

在服务器初始化期间逐步预热缓存：

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

在首次请求文件时预加载：

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

监控缓存效果：

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

1. **从简单开始**：先使用目录级预热，再考虑复杂策略
2. **关注内存**：跟踪缓存用量，避免过度消耗内存
3. **设置优先级**：为频繁访问的文件设置更高优先级
4. **有所取舍**：不要预热所有内容——聚焦关键资源
5. **性能剖析**：使用缓存统计找出热点文件并调整策略

### 何时使用预热

- **生产部署**：在接收流量前预加载关键资源
- **零停机部署**：在路由流量前预热新实例
- **高流量事件**：为预期峰值准备缓存
- **性能调优**：根据访问模式优化

### 何时不应使用预热

- **开发阶段**：开发时无此必要
- **小型应用**：缓存可能没有帮助
- **动态生成内容**：静态缓存无济于事
- **内存受限环境**：可能导致内存压力

## 故障排除

### 常见问题

1. **404 错误**
   - 检查文件路径是否正确
   - 确认文件存在于根目录下
   - 验证文件权限

2. **403 错误**
   - 检查文件权限设置
   - 确认文件类型在允许列表中
   - 验证路径安全性

3. **性能问题**
   - 增加缓存大小
   - 启用文件压缩
   - 检查磁盘 I/O 性能

4. **内存使用过高**
   - 减少缓存大小
   - 缩短缓存 TTL
   - 监控内存使用情况

### 调试技巧

```c
g_error_config.min_logLevel = UVHTTP_LOG_LEVEL_DEBUG;

UVHTTP_LOG_INFO("处理请求: %s %s",
                uvhttp_request_get_method(request),
                uvhttp_request_get_url(request));
```

## 示例项目

- `examples/static_file_server.c` - 基础静态文件服务器
- `examples/cache_test_server.c` - 缓存功能测试服务器

## 版本历史

- **v1.0.0** - 初始版本，基础静态文件服务功能
- **v1.1.0** - 添加 LRU 缓存支持
- **v1.2.0** - 增强安全特性和性能优化

## 许可证

MIT License - 详见 LICENSE 文件
