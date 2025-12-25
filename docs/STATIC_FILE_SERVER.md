# UVHTTP 静态文件服务器指南

## 概述

UVHTTP静态文件服务器是一个高性能、安全、易于使用的静态文件服务解决方案。它提供了完整的静态文件服务功能，包括自动MIME类型检测、文件缓存、条件请求支持等特性。

## 核心特性

### 🚀 性能优化
- **LRU缓存系统**: 智能内存缓存，减少磁盘I/O
- **零拷贝优化**: 高效的文件传输机制
- **连接复用**: 基于libuv的事件驱动架构
- **压缩支持**: 预留gzip/deflate压缩接口

### 🔒 安全特性
- **路径安全验证**: 防止目录遍历攻击
- **文件类型检查**: 可配置的文件类型白名单
- **访问控制**: 支持基于路径的访问限制
- **资源限制**: 防止大文件DoS攻击

### 📊 功能特性
- **自动MIME类型检测**: 支持常见文件类型
- **条件请求**: ETag和Last-Modified支持
- **目录列表**: 可配置的目录浏览功能
- **自定义头部**: 支持添加自定义HTTP头部
- **错误处理**: 友好的错误页面和日志记录

## 快速开始

### 基础示例

```c
#include "uvhttp.h"
#include "uvhttp_static.h"

int main() {
    // 配置静态文件服务
    uvhttp_static_config_t config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB缓存
        .cache_ttl = 3600                      // 1小时TTL
    };
    
    // 创建静态文件服务上下文
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    
    // 创建服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 设置静态文件处理器
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/*", static_file_handler);
    server->router = router;
    
    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

### 高级配置

```c
// 高级配置示例
uvhttp_static_config_t advanced_config = {
    .root_directory = "/var/www/html",
    .index_file = "index.html",
    .enable_directory_listing = 0,  // 禁用目录列表
    .enable_etag = 1,
    .enable_last_modified = 1,
    .max_cache_size = 100 * 1024 * 1024,  // 100MB缓存
    .cache_ttl = 7200,                      // 2小时TTL
    .custom_headers = "X-Content-Type-Options: nosniff\r\n"
                     "X-Frame-Options: DENY\r\n"
                     "X-XSS-Protection: 1; mode=block",
    .allowed_extensions = ".html,.css,.js,.png,.jpg,.gif,.ico,.svg",
    .max_file_size = 50 * 1024 * 1024,     // 50MB文件大小限制
    .enable_compression = 1                 // 启用压缩
};
```

## 配置选项详解

### 基础配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `root_directory` | `const char*` | `"./public"` | 静态文件根目录 |
| `index_file` | `const char*` | `"index.html"` | 默认首页文件 |
| `enable_directory_listing` | `int` | `1` | 是否启用目录列表 |
| `enable_etag` | `int` | `1` | 是否启用ETag支持 |
| `enable_last_modified` | `int` | `1` | 是否启用Last-Modified支持 |

### 缓存配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `max_cache_size` | `size_t` | `10*1024*1024` | 最大缓存大小（字节） |
| `cache_ttl` | `int` | `3600` | 缓存TTL（秒） |

### 安全配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `allowed_extensions` | `const char*` | `NULL` | 允许的文件扩展名 |
| `max_file_size` | `size_t` | `50*1024*1024` | 最大文件大小 |
| `enable_compression` | `int` | `0` | 是否启用压缩 |

### HTTP配置

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `custom_headers` | `const char*` | `""` | 自定义HTTP头部 |

## API参考

### 核心函数

#### `uvhttp_static_context_t* uvhttp_static_create(const uvhttp_static_config_t* config)`
创建静态文件服务上下文。

**参数:**
- `config`: 静态文件配置

**返回值:**
- 成功: 静态文件服务上下文指针
- 失败: `NULL`

#### `void uvhttp_static_free(uvhttp_static_context_t* ctx)`
释放静态文件服务上下文。

**参数:**
- `ctx`: 静态文件服务上下文

#### `int uvhttp_static_handle_request(uvhttp_static_context_t* ctx, uvhttp_request_t* request, uvhttp_response_t* response)`
处理静态文件请求。

**参数:**
- `ctx`: 静态文件服务上下文
- `request`: HTTP请求对象
- `response`: HTTP响应对象

**返回值:**
- `0`: 成功
- `非0`: 错误码

### 工具函数

#### `const char* uvhttp_static_get_mime_type(const char* file_path)`
根据文件路径获取MIME类型。

#### `int uvhttp_static_is_safe_path(const char* root_dir, const char* file_path)`
检查文件路径是否安全（防止目录遍历）。

#### `char* uvhttp_static_generate_etag(const char* file_path, size_t file_size, time_t mtime)`
为文件生成ETag值。

## 最佳实践

### 1. 目录结构

```
project/
├── public/                 # 静态文件根目录
│   ├── css/               # 样式文件
│   ├── js/                # JavaScript文件
│   ├── images/            # 图片文件
│   ├── fonts/             # 字体文件
│   └── docs/              # 文档文件
├── src/                   # 源代码
└── server.c               # 服务器主程序
```

### 2. 安全配置

```c
// 生产环境安全配置
uvhttp_static_config_t secure_config = {
    .root_directory = "/var/www/html",
    .enable_directory_listing = 0,  // 禁用目录列表
    .allowed_extensions = ".html,.css,.js,.png,.jpg,.gif,.ico,.svg,.woff,.woff2",
    .max_file_size = 10 * 1024 * 1024,  // 10MB限制
    .custom_headers = 
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: DENY\r\n"
        "X-XSS-Protection: 1; mode=block\r\n"
        "Strict-Transport-Security: max-age=31536000; includeSubDomains"
};
```

### 3. 性能优化

```c
// 高性能配置
uvhttp_static_config_t perf_config = {
    .max_cache_size = 500 * 1024 * 1024,  // 500MB缓存
    .cache_ttl = 86400,                     // 24小时TTL
    .enable_compression = 1,                // 启用压缩
    .enable_etag = 1,                       // 启用ETag
    .enable_last_modified = 1               // 启用Last-Modified
};
```

### 4. 错误处理

```c
void static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    int result = uvhttp_static_handle_request(g_static_ctx, request, response);
    
    if (result != 0) {
        // 根据错误类型返回不同的错误页面
        switch (result) {
            case UVHTTP_STATIC_ERROR_NOT_FOUND:
                serve_404_page(response);
                break;
            case UVHTTP_STATIC_ERROR_FORBIDDEN:
                serve_403_page(response);
                break;
            case UVHTTP_STATIC_ERROR_TOO_LARGE:
                serve_413_page(response);
                break;
            default:
                serve_500_page(response);
                break;
        }
    }
    
    uvhttp_response_send(response);
}
```

## 监控和调试

### 日志记录

UVHTTP静态文件服务器提供了详细的日志记录功能：

```bash
# 启用调试日志
export UVHTTP_LOG_LEVEL=DEBUG

# 运行服务器
./static_file_server
```

### 性能监控

```c
// 获取缓存统计信息
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

1. **404错误**
   - 检查文件路径是否正确
   - 确认文件存在于根目录下
   - 验证文件权限

2. **403错误**
   - 检查文件权限设置
   - 确认文件类型在允许列表中
   - 验证路径安全性

3. **性能问题**
   - 增加缓存大小
   - 启用文件压缩
   - 检查磁盘I/O性能

4. **内存使用过高**
   - 减少缓存大小
   - 缩短缓存TTL
   - 监控内存使用情况

### 调试技巧

```c
// 启用详细日志
g_error_config.min_logLevel = UVHTTP_LOG_LEVEL_DEBUG;

// 添加自定义日志
UVHTTP_LOG_INFO("处理请求: %s %s", 
                uvhttp_request_get_method(request),
                uvhttp_request_get_url(request));
```

## 示例项目

完整的工作示例请参考：
- `examples/static_file_server.c` - 基础静态文件服务器
- `examples/cache_test_server.c` - 缓存功能测试服务器

## 版本历史

- **v1.0.0** - 初始版本，基础静态文件服务功能
- **v1.1.0** - 添加LRU缓存支持
- **v1.2.0** - 增强安全特性和性能优化

## 许可证

MIT License - 详见LICENSE文件