# UVHTTP 开发指南

## 目录
1. [设计原则](#设计原则)
2. [开发规范](#开发规范)
3. [快速开始](#快速开始)
4. [配置管理](#配置管理)
5. [API 参考](#api-参考)
6. [示例程序](#示例程序)
7. [测试指南](#测试指南)
8. [性能优化](#性能优化)
9. [常见问题](#常见问题)
10. [贡献指南](#贡献指南)
11. [许可证](#许可证)

## 设计原则

### 🎯 核心设计理念

UVHTTP 遵循以下核心设计原则，确保项目既高性能又易于维护：

#### 1. **零开销抽象 (Zero-Cost Abstraction)**

**概念**：所有抽象通过编译时实现，运行时无额外开销

**实现方式**：
```c
// 编译时选择分配器类型
#if UVHTTP_ALLOCATOR_TYPE == 1
    #define UVHTTP_MALLOC(size) mi_malloc(size)  // 直接调用
#else
    #define UVHTTP_MALLOC(size) malloc(size)     // 直接调用
#endif
```

**优势**：
- 生产环境性能等同于直接调用
- 编译器可进行充分优化
- 无运行时多态开销

#### 2. **超轻量级架构**

**目标**：最小化依赖和内存占用

**实现策略**：
- 仅依赖必要的库（libuv、llhttp）
- 紧凑的数据结构设计
- 避免过度工程化

**示例**：
```c
// 紧凑的响应结构
typedef struct uvhttp_response {
    uv_tcp_t* client;
    int status_code;
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    char* body;
    size_t body_length;
    int keep_alive;
    int sent;
    int finished;
} uvhttp_response_t;
```

#### 3. **性能优先原则**

**优先级**：性能 > 功能 > 代码量

**具体措施**：
- 内存分配器零开销设计
- 网络层直接使用 libuv
- 避免不必要的数据拷贝
- 使用编译器内联优化

#### 4. **可测试性设计**

**策略**：在不影响性能的前提下提供测试支持

**实现**：
- 纯函数与副作用分离
- 编译时测试宏控制
- 模拟对象支持

### 🏗️ 架构设计原则

#### 1. **模块化设计**

**原则**：每个模块职责单一，接口清晰

**目录结构**：
```
uvhttp/
├── include/           # 公共头文件
├── src/              # 源代码实现
├── examples/         # 示例代码
├── test/             # 测试代码
└── docs/             # 文档
```

**接口设计**：
```c
// 不暴露内部结构细节
typedef struct uvhttp_response uvhttp_response_t;

// 清晰的函数命名
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status);
```

#### 2. **编译时配置**

**理念**：尽可能在编译时做决策，减少运行时开销

**功能开关**：
```c
// 功能特性编译时控制
#ifndef UVHTTP_FEATURE_WEBSOCKET
#define UVHTTP_FEATURE_WEBSOCKET 1
#endif

// 分配器类型选择
#ifndef UVHTTP_ALLOCATOR_TYPE
#define UVHTTP_ALLOCATOR_TYPE 0
#endif
```

#### 3. **错误处理统一**

**策略**：使用统一的错误类型和处理方式

**错误类型**：
```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NETWORK = -3,
    // ...
} uvhttp_error_t;
```

**错误检查宏**：
```c
#define UVHTTP_RETURN_IF_ERROR(expr) \
    do { \
        int _err = (expr); \
        if (UVHTTP_UNLIKELY(_err != 0)) return _err; \
    } while(0)
```

## 开发规范

### 🚀 API 使用规范

#### 1. **统一使用核心API**

UVHTTP 采用统一的核心API设计，不再支持多层次的API抽象。所有开发应直接使用核心API：

**推荐用法**：
```c
int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 设置状态码
    uvhttp_response_set_status(res, 200);
    
    // 设置响应头
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    
    // 设置响应体
    const char* body = "{\"message\":\"Hello World\"}";
    uvhttp_response_set_body(res, body, strlen(body));
    
    // 发送响应
    return uvhttp_response_send(res);
}
```

**已废弃的API**（不再使用）：
- `uvhttp_api_*` 系列函数
- `uvhttp_serve()` 一行启动函数
- `uvhttp_send_json_response()` 等便捷函数
- 响应构建器模式

#### 2. **服务器创建标准模式**

```c
// 标准服务器创建流程
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;

// 添加路由
uvhttp_router_add_route(router, "/api", api_handler);

// 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);
uv_run(loop, UV_RUN_DEFAULT);
```

#### 3. **响应处理标准模式**

所有响应处理应遵循以下模式：
1. 设置状态码（`uvhttp_response_set_status`）
2. 设置响应头（`uvhttp_response_set_header`）
3. 设置响应体（`uvhttp_response_set_body`）
4. 发送响应（`uvhttp_response_send`）

### 📝 代码规范

#### 1. **命名约定**

**文件命名**：
- 头文件：`uvhttp_模块名.h` (如 `uvhttp_response.h`)
- 源文件：`uvhttp_模块名.c` (如 `uvhttp_response.c`)
- 测试文件：`test_功能名.c` (如 `test_response.c`)
- 示例文件：`功能名_demo.c` (如 `helloworld_demo.c`)

**类型命名**：
```c
// 结构体类型定义
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_request uvhttp_request_t;

// 枚举类型
typedef enum {
    UVHTTP_STATE_INIT,
    UVHTTP_STATE_RUNNING,
    UVHTTP_STATE_STOPPED
} uvhttp_state_t;

// 常量命名
#define UVHTTP_MAX_HEADERS 64
#define UVHTTP_DEFAULT_PORT 8080
```

**函数命名**：
```c
// 公共API函数：uvhttp_模块_动作
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status);
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);

// 静态辅助函数：模块化命名
static void build_response_headers(uvhttp_response_t* response, char* buffer, size_t* length);
static int validate_header_value(const char* value);
```

#### 2. **代码组织**

**文件结构顺序**：
```c
// 1. 版权和许可证信息
// 2. 头文件包含
#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include <uv.h>

// 3. 宏定义和常量
#define MAX_BUFFER_SIZE 8192

// 4. 类型定义
typedef struct {
    // ...
} internal_struct_t;

// 5. 静态函数声明
static void helper_function(void);
static int validate_input(const char* input);

// 6. 公共API实现
uvhttp_error_t uvhttp_public_function(void) {
    // 实现
}

// 7. 静态函数实现
static void helper_function(void) {
    // 实现
}
```

#### 3. **注释规范**

**文件头注释**：
```c
/**
 * @file uvhttp_response.c
 * @brief HTTP响应处理模块实现
 * @author UVHTTP Team
 * @date 2025
 * 
 * 本模块提供HTTP响应的构建、发送和管理功能。
 * 遵循零开销抽象原则，性能优先。
 */
```

**函数注释**：
```c
/**
 * @brief 设置HTTP响应状态码
 * @param response 响应对象指针，不能为NULL
 * @param status_code HTTP状态码，范围100-599
 * @return UVHTTP_OK 成功，其他值表示错误
 * 
 * @note 此函数为纯函数，无副作用，便于测试
 * @see uvhttp_response_set_header
 */
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
```

#### 4. **内存管理规范**

**分配原则**：
```c
// 使用统一的分配器宏
void* ptr = UVHTTP_MALLOC(size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// 确保释放
UVHTTP_FREE(ptr);
```

**RAII 风格**：
```c
// 资源获取即初始化
uvhttp_response_t* response = uvhttp_response_new();
if (!response) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// 使用资源
// ...

// 确保清理
uvhttp_response_free(response);
```

### 🔧 性能编码规范

#### 1. **编译器优化**

**内联函数**：
```c
// 关键路径函数使用内联
static inline UVHTTP_INLINE int fast_path_function(void) {
    // 简单操作，编译器内联
    return result;
}
```

**分支预测**：
```c
// 使用分支预测宏
#define UVHTTP_LIKELY(x) __builtin_expect(!!(x), 1)
#define UVHTTP_UNLIKELY(x) __builtin_expect(!!(x), 0)

if (UVHTTP_LIKELY(response != NULL)) {
    // 常见路径
} else {
    // 异常路径
}
```

#### 2. **内存优化**

**栈分配优先**：
```c
// 优先使用栈分配
char buffer[UVHTTP_STACK_BUFFER_SIZE];

// 大内存才使用堆分配
if (size > UVHTTP_STACK_BUFFER_SIZE) {
    buffer = UVHTTP_MALLOC(size);
}
```

**内存对齐**：
```c
// 确保结构体对齐
typedef struct UVHTTP_ALIGNED(16) {
    // 频繁访问的数据
} aligned_struct_t;
```

#### 3. **系统调用优化**

**批量操作**：
```c
// 批量发送headers，减少系统调用
int uvhttp_response_send_headers_batch(uvhttp_response_t* response) {
    // 一次性发送所有headers
}
```

**异步优先**：
```c
// 使用异步I/O，避免阻塞
uv_write(&write_req, stream, &buf, 1, callback);
```

### 🧪 测试规范

#### 1. **测试结构**

**测试文件组织**：
```c
// 测试文件结构
#include "uvhttp_test_helpers.h"

// 测试用例
static int test_function_name(void) {
    // 测试代码
    return 0;  // 成功返回0
}

// 主测试函数
int main(void) {
    UVHTTP_TEST_ASSERT_SUCCESS(uvhttp_test_env_init());
    
    int result = 0;
    result |= test_function_name();
    
    uvhttp_test_env_cleanup();
    return result;
}
```

#### 2. **断言使用**

**标准断言**：
```c
// 使用测试专用断言
UVHTTP_TEST_ASSERT(condition);
UVHTTP_TEST_ASSERT_EQ(expected, actual);
UVHTTP_TEST_ASSERT_NOT_NULL(ptr);
UVHTTP_TEST_ASSERT_SUCCESS(error_code);
```

#### 3. **性能测试**

**基准测试**：
```c
// 性能测试模式
UVHTTP_PERF_START(operation_name);
for (int i = 0; i < iterations; i++) {
    operation();
}
UVHTTP_PERF_END(operation_name);
```

### 📋 项目管理规范

#### 1. **版本控制**

**提交信息格式**：
```
类型(范围): 简短描述

详细描述（可选）

相关问题: #123
```

**分支命名**：
- `feature/功能名称` - 新功能开发
- `fix/问题描述` - 错误修复
- `perf/优化描述` - 性能优化

#### 2. **代码审查**

**审查要点**：
- 性能影响评估
- 内存安全性检查
- API兼容性验证
- 测试覆盖率确认

#### 3. **发布流程**

**发布检查清单**：
- [ ] 所有测试通过
- [ ] 性能基准测试通过
- [ ] 内存泄漏检查通过
- [ ] 文档更新完成
- [ ] 版本号更新

### 🎯 质量保证

#### 1. **静态分析**

**编译警告**：
```bash
# 启用所有警告
gcc -Wall -Wextra -Werror -std=c11
```

**代码检查**：
```bash
# 使用 cppcheck
cppcheck --enable=all --std=c11 src/
```

#### 2. **内存安全**

**检测工具**：
```bash
# 内存泄漏检测
valgrind --leak-check=full ./test_program

# 地址消毒器
gcc -fsanitize=address -g -o test test.c
./test
```

#### 3. **性能监控**

**基准测试**：
```bash
# 运行性能基准
./test_allocator_performance

# 性能回归测试
make -f test/Makefile.allocator compare
```

这些设计原则和开发规范确保了 UVHTTP 项目的高质量、高性能和可维护性。所有开发者都应遵循这些规范，以保持项目的一致性和卓越性能。

## 快速开始

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libuv-dev libmbedtls-dev

# CentOS/RHEL
sudo yum install libuv-devel mbedtls-devel

# macOS
brew install libuv mbedtls
```

### 编译项目

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 运行第一个服务器

```c
#include "uvhttp.h"

void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## 配置管理

UVHTTP提供了灵活的配置管理系统，支持多种配置方式和运行时动态调整。本章重点介绍并发连接数等关键配置的管理方法。

### 配置结构

UVHTTP使用 `uvhttp_config_t` 结构体来管理所有配置参数：

```c
typedef struct {
    /* 服务器配置 */
    int max_connections;              // 最大并发连接数 (1-10000)
    int max_requests_per_connection;  // 每个连接的最大请求数
    int backlog;                      // 监听队列大小
    
    /* 性能配置 */
    size_t max_body_size;             // 最大请求体大小
    size_t max_header_size;           // 最大请求头大小
    int read_buffer_size;             // 读取缓冲区大小
    
    /* 安全配置 */
    int rate_limit_window;            // 速率限制窗口时间
    int enable_compression;           // 是否启用压缩
    int enable_tls;                   // 是否启用TLS
    
    /* 其他配置... */
} uvhttp_config_t;
```

### 配置方式

#### 1. 代码配置

直接在代码中设置配置参数：

```c
#include "uvhttp_config.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建配置对象
    uvhttp_config_t* config = uvhttp_config_new();
    
    // 设置并发连接数限制
    config->max_connections = 3000;           // 最大3000个并发连接
    config->max_requests_per_connection = 200; // 每个连接最多200个请求
    
    // 其他性能配置
    config->max_body_size = 2 * 1024 * 1024;  // 2MB最大请求体
    config->read_buffer_size = 16384;          // 16KB读取缓冲区
    
    // 应用配置
    server->config = config;
    
    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

#### 2. 配置文件

创建配置文件 `uvhttp.conf`：

```
# 服务器配置
max_connections=3000
max_requests_per_connection=200
backlog=1024

# 性能配置
max_body_size=2097152
max_header_size=16384
read_buffer_size=16384

# 安全配置
rate_limit_window=60
enable_compression=1
enable_tls=0
```

在代码中加载配置文件：

```c
uvhttp_config_t* config = uvhttp_config_new();

// 从文件加载配置
if (uvhttp_config_load_file(config, "uvhttp.conf") != UVHTTP_OK) {
    fprintf(stderr, "配置文件加载失败，使用默认配置\n");
    uvhttp_config_set_defaults(config);
}

// 验证配置
if (uvhttp_config_validate(config) != UVHTTP_OK) {
    fprintf(stderr, "配置参数无效\n");
    return 1;
}

server->config = config;
```

#### 3. 环境变量配置

通过环境变量设置配置：

```bash
# 设置环境变量
export UVHTTP_MAX_CONNECTIONS=4000
export UVHTTP_MAX_REQUESTS_PER_CONNECTION=150
export UVHTTP_ENABLE_COMPRESSION=1

# 运行服务器
./your_server
```

在代码中加载环境变量：

```c
uvhttp_config_t* config = uvhttp_config_new();
uvhttp_config_set_defaults(config);

// 加载环境变量配置（会覆盖默认值）
uvhttp_config_load_env(config);
```

### 动态配置调整

UVHTTP支持运行时动态调整配置，无需重启服务器：

#### 动态调整连接数限制

```c
// 根据系统负载动态调整连接数
void adjust_connections_dynamically() {
    double cpu_usage = get_cpu_usage();
    double memory_usage = get_memory_usage();
    
    if (cpu_usage > 0.8 || memory_usage > 0.8) {
        // 系统负载高，降低连接数
        int new_limit = get_current_connections() * 0.8;
        uvhttp_config_update_max_connections(new_limit);
        printf("高负载，连接数限制调整为: %d\n", new_limit);
    } else if (cpu_usage < 0.3 && memory_usage < 0.3) {
        // 系统负载低，增加连接数
        int current = get_current_max_connections();
        int new_limit = current * 1.2;
        if (new_limit <= 10000) {  // 不超过最大限制
            uvhttp_config_update_max_connections(new_limit);
            printf("低负载，连接数限制调整为: %d\n", new_limit);
        }
    }
}

// 定时调整配置
void config_adjustment_timer(uv_timer_t* handle) {
    adjust_connections_dynamically();
}

int main() {
    // ... 服务器初始化代码 ...
    
    // 设置定时器，每30秒检查一次系统负载
    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, timer);
    uv_timer_start(timer, config_adjustment_timer, 30000, 30000);
    
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

#### 配置变化监控

```c
// 配置变化回调函数
void on_config_change(const char* key, const void* old_value, const void* new_value) {
    printf("配置变化: %s\n", key);
    
    if (strcmp(key, "max_connections") == 0) {
        int old_conn = *(const int*)old_value;
        int new_conn = *(const int*)new_value;
        printf("最大连接数: %d -> %d\n", old_conn, new_conn);
        
        // 可以在这里执行相关逻辑，如通知监控系统
        notify_monitoring_system("connection_limit_changed", new_conn);
    }
}

// 启用配置监控
void setup_config_monitoring() {
    uvhttp_config_monitor_changes(on_config_change);
}
```

### 并发连接数最佳实践

#### 1. 合理设置连接数限制

根据系统资源和应用特点设置合适的连接数：

```c
// 根据系统内存计算合理的连接数
int calculate_optimal_connections() {
    size_t system_memory_mb = get_system_memory_mb();
    size_t memory_per_connection_mb = 2;  // 每个连接大约需要2MB内存
    
    // 使用系统内存的70%用于连接处理
    int optimal_connections = (system_memory_mb * 0.7) / memory_per_connection_mb;
    
    // 确保在合理范围内
    if (optimal_connections < 100) optimal_connections = 100;
    if (optimal_connections > 10000) optimal_connections = 10000;
    
    return optimal_connections;
}

// 应用启动时自动优化配置
void optimize_server_config(uvhttp_config_t* config) {
    int optimal_connections = calculate_optimal_connections();
    config->max_connections = optimal_connections;
    
    printf("系统内存: %zuMB, 推荐最大连接数: %d\n", 
           get_system_memory_mb(), optimal_connections);
}
```

#### 2. 连接数监控和告警

```c
// 连接数监控结构
typedef struct {
    int current_connections;
    int max_connections;
    time_t last_warning_time;
} connection_monitor_t;

static connection_monitor_t g_conn_monitor = {0};

// 连接数监控函数
void monitor_connections(uvhttp_server_t* server) {
    g_conn_monitor.current_connections = server->active_connections;
    g_conn_monitor.max_connections = server->config->max_connections;
    
    double usage_ratio = (double)g_conn_monitor.current_connections / g_conn_monitor.max_connections;
    
    // 连接数使用率超过80%时发出警告
    if (usage_ratio > 0.8) {
        time_t now = time(NULL);
        // 避免频繁告警，每5分钟最多告警一次
        if (now - g_conn_monitor.last_warning_time > 300) {
            printf("警告: 连接数使用率 %.1f%% (%d/%d)\n", 
                   usage_ratio * 100, 
                   g_conn_monitor.current_connections, 
                   g_conn_monitor.max_connections);
            
            // 可以发送告警邮件或通知监控系统
            send_alert("connection_usage_high", usage_ratio);
            g_conn_monitor.last_warning_time = now;
        }
    }
}

// 在服务器处理连接时调用监控函数
void on_connection_event(uvhttp_server_t* server) {
    monitor_connections(server);
}
```

#### 3. 连接池管理

```c
// 连接池配置
typedef struct {
    int min_connections;      // 最小连接数
    int max_connections;      // 最大连接数
    int idle_timeout;         // 空闲超时时间
    int active_connections;   // 当前活跃连接数
} connection_pool_t;

// 连接池管理
void manage_connection_pool(uvhttp_server_t* server) {
    connection_pool_t* pool = server->connection_pool;
    
    // 如果连接数过少，可以考虑预热连接
    if (pool->active_connections < pool->min_connections) {
        // 预热连接逻辑
        warmup_connections(pool->min_connections - pool->active_connections);
    }
    
    // 如果连接数接近上限，可以采取限流措施
    if (pool->active_connections > pool->max_connections * 0.9) {
        enable_rate_limiting();
    }
}
```

### 配置验证

确保配置参数在合理范围内：

```c
int validate_server_config(const uvhttp_config_t* config) {
    // 验证连接数配置
    if (config->max_connections < 1 || config->max_connections > 10000) {
        fprintf(stderr, "错误: max_connections 必须在 1-10000 范围内\n");
        return -1;
    }
    
    if (config->max_requests_per_connection < 1 || 
        config->max_requests_per_connection > 10000) {
        fprintf(stderr, "错误: max_requests_per_connection 必须在 1-10000 范围内\n");
        return -1;
    }
    
    // 验证缓冲区大小
    if (config->read_buffer_size < 1024 || config->read_buffer_size > 1024 * 1024) {
        fprintf(stderr, "错误: read_buffer_size 必须在 1KB-1MB 范围内\n");
        return -1;
    }
    
    // 验证内存配置
    size_t total_memory_needed = config->max_connections * 
                                (config->read_buffer_size + config->max_body_size);
    size_t system_memory = get_system_memory();
    
    if (total_memory_needed > system_memory * 0.8) {
        fprintf(stderr, "警告: 配置的内存需求可能超过系统可用内存\n");
        fprintf(stderr, "需求内存: %zuMB, 系统内存: %zuMB\n", 
               total_memory_needed / (1024 * 1024), 
               system_memory / (1024 * 1024));
    }
    
    return 0;
}
```

### 故障排除

#### 常见配置问题

1. **连接被拒绝**
   - 检查 `max_connections` 是否设置过小
   - 查看日志中的 "Connection limit reached" 警告
   - 考虑动态调整连接数限制

2. **内存不足**
   - 降低 `max_connections` 值
   - 减少 `read_buffer_size` 或 `max_body_size`
   - 监控系统内存使用情况

3. **性能问题**
   - 根据系统资源调整连接数
   - 启用连接池管理
   - 考虑负载均衡

#### 调试配置

```c
// 打印当前配置信息
void print_current_config() {
    const uvhttp_config_t* config = uvhttp_config_get_current();
    
    printf("=== 当前配置 ===\n");
    printf("最大连接数: %d\n", config->max_connections);
    printf("每连接最大请求数: %d\n", config->max_requests_per_connection);
    printf("最大请求体大小: %zuMB\n", config->max_body_size / (1024 * 1024));
    printf("读取缓冲区大小: %zuKB\n", config->read_buffer_size / 1024);
    printf("启用压缩: %s\n", config->enable_compression ? "是" : "否");
    printf("启用TLS: %s\n", config->enable_tls ? "是" : "否");
    printf("================\n");
}
```

## API 参考

### 服务器管理

#### `uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop)`
创建新的 HTTP 服务器实例。

**参数:**
- `loop`: libuv 事件循环

**返回:**
- 成功返回服务器实例指针，失败返回 NULL

#### `int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port)`
启动服务器监听指定端口。

**参数:**
- `server`: 服务器实例
- `host`: 监听地址
- `port`: 监听端口

**返回:**
- 成功返回 0，失败返回负数

### 请求处理

#### `const char* uvhttp_request_get_method(uvhttp_request_t* request)`
获取 HTTP 请求方法。

#### `const char* uvhttp_request_get_url(uvhttp_request_t* request)`
获取请求 URL。

#### `const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name)`
获取请求头。

### 响应处理

#### `void uvhttp_response_set_status(uvhttp_response_t* response, int status_code)`
设置 HTTP 响应状态码。

#### `void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)`
设置响应头。

#### `int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)`
设置响应体。

## 示例程序

### 1. 简单 HTTP 服务器

位置: `examples/simple_server.c`

功能:
- 基本路由处理
- HTML 响应
- 错误处理
- 日志记录

运行:
```bash
cd build
./simple_server
```

访问:
- http://localhost:8080/ - 主页
- http://localhost:8080/api - JSON API
- http://localhost:8080/info - 服务器信息

### 2. RESTful API 服务器

位置: `examples/restful_api_server.c`

功能:
- CRUD 操作
- JSON 处理
- CORS 支持
- 中间件

API 端点:
- `GET /tasks` - 获取所有任务
- `GET /tasks/{id}` - 获取单个任务
- `POST /tasks` - 创建任务
- `PUT /tasks/{id}` - 更新任务
- `DELETE /tasks/{id}` - 删除任务

### 3. WebSocket 服务器

位置: `examples/websocket_example.c`

功能:
- WebSocket 连接
- 消息广播
- 心跳检测

## 测试指南

### 运行测试套件

```bash
# 运行所有测试
make test

# 运行综合测试
./build/comprehensive_test_suite

# 运行 WebSocket 测试
./build/websocket_integration_test

# 运行压力测试
cd test && ./run_stress_tests.sh
```

### 测试类型

1. **单元测试**
   - 请求处理测试
   - 响应处理测试
   - 路由系统测试
   - 内存管理测试

2. **集成测试**
   - 完整请求-响应流程
   - WebSocket 连接测试
   - 错误处理测试

3. **性能测试**
   - 内存分配性能
   - 字符串处理性能
   - 并发连接测试

4. **压力测试**
   - 高并发连接
   - 长时间运行
   - 内存泄漏检测

### 编写测试

测试文件命名规范: `test_*.c`

测试函数命名规范: `test_*()`

```c
void test_feature_name() {
    /* 测试代码 */
    TEST_ASSERT(condition, "测试描述");
}
```

## 性能优化

### 内存管理

UVHTTP 提供了多种内存分配器选项：

```c
// 使用系统分配器（默认）
#define UVHTTP_ALLOCATOR_TYPE 0

// 使用 mimalloc
#define UVHTTP_ALLOCATOR_TYPE 1

// 使用自定义分配器
#define UVHTTP_ALLOCATOR_TYPE 2
```

### 连接优化

```c
// 设置最大连接数
server->max_connections = 1000;

// 设置读取缓冲区大小
server->read_buffer_size = 8192;
```

### 错误恢复

```c
// 配置重试策略
uvhttp_set_error_recovery_config(
    3,      // 最大重试次数
    100,    // 基础延迟 (ms)
    5000,   // 最大延迟 (ms)
    2.0     // 退避倍数
);
```

## 常见问题

### Q: 如何处理静态文件？

A: 使用内置的静态文件中间件：

```c
void static_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_url(request);
    uvhttp_serve_static_file(response, path, "./public");
}
```

### Q: 如何启用 HTTPS？

A: 配置 TLS 上下文：

```c
uvhttp_tls_config_t tls_config = {
    .cert_file = "server.crt",
    .key_file = "server.key"
};

uvhttp_server_enable_tls(server, &tls_config);
```

### Q: 如何处理大文件上传？

A: 配置请求体大小限制：

```c
// 设置最大请求体大小 (10MB)
server->max_body_size = 10 * 1024 * 1024;
```

### Q: 如何添加中间件？

A: 在路由处理器中调用中间件函数：

```c
void auth_middleware(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* auth = uvhttp_request_get_header(request, "Authorization");
    if (!auth || !validate_token(auth)) {
        uvhttp_response_set_status(response, 401);
        uvhttp_response_send(response);
        return;
    }
    // 继续处理
}
```

### Q: 如何调试内存问题？

A: 启用内存调试模式：

```c
#define UVHTTP_ENABLE_MEMORY_DEBUG

// 获取内存统计
size_t total, current, alloc_count, free_count;
uvhttp_get_memory_stats(&total, &current, &alloc_count, &free_count);

// 检查内存泄漏
if (uvhttp_check_memory_leaks()) {
    printf("检测到内存泄漏\n");
}
```

## 贡献指南

1. Fork 项目
2. 创建功能分支
3. 编写测试
4. 提交 Pull Request

## 许可证

MIT License - 详见 LICENSE 文件