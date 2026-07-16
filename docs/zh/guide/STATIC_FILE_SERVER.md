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

#### `const char* uvhttp_static_get_mime_type(const char* file_path)`

根据文件路径获取 MIME 类型。

#### `int uvhttp_static_is_safe_path(const char* root_dir, const char* file_path)`

检查文件路径是否安全（防止目录遍历）。

#### `char* uvhttp_static_generate_etag(const char* file_path, size_t file_size, time_t mtime)`

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
- 支持添加新的 MIME 类型映射

### 扩展控制

1. **添加自定义 MIME 类型**：
```c
uvhttp_add_mime_mapping(".custom", "application/custom");
```

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
