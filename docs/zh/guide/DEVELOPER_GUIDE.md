# UVHTTP 贡献者指南

## 概述

本文档面向 UVHTTP 库的贡献者，介绍如何参与库的开发、代码规范、最佳实践等。

## 开发环境

### 系统要求

- **操作系统**: Linux, macOS, Windows
- **编译器**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **CMake**: 3.10+
- **依赖库**:
  - libuv 1.x
  - mbedtls 2.x (可选，用于 TLS)
  - mimalloc (可选，高性能内存分配器)

### 安装依赖

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential cmake libuv-dev libmbedtls-dev
```

#### CentOS/RHEL

```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake libuv-devel mbedtls-devel
```

#### macOS

```bash
brew install cmake libuv mbedtls
```

## 编译项目

### 构建模式规范

UVHTTP 项目定义了三种构建模式，每种模式适用于不同的场景：

| 构建模式 | 用途 | 编译选项 | 适用程序 |
|----------|------|----------|----------|
| **Release** | 生产环境、性能测试 | `-O2 -DNDEBUG` | 所有 benchmark 程序、示例程序 |
| **Debug** | 开发调试、单元测试 | `-O0 -g` | 单元测试、测试程序 |
| **Coverage** | 代码覆盖率分析 | `-O0 --coverage` | 覆盖率测试 |

**重要提示**：
- ⚠️ **性能测试必须使用 Release 模式**，否则数据不准确（Debug 模式性能可能低 10-100 倍）
- ✅ 详细的构建模式规范请参考 [BUILD_MODES.md](../dev/BUILD_MODES.md)

### 基本编译

#### Release 模式（推荐用于性能测试）

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### Debug 模式（用于开发调试）

```bash
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make -j$(nproc)
```

#### Coverage 模式（用于覆盖率分析）

```bash
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make -j$(nproc)
```

### 编译选项

```bash
# 启用 WebSocket 支持
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 启用 mimalloc 分配器
cmake -DBUILD_WITH_MIMALLOC=ON ..

# 启用 TLS 支持
cmake -DBUILD_WITH_HTTPS=ON ..

# Debug 模式
cmake -DENABLE_DEBUG=ON ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 启用示例程序
cmake -DBUILD_EXAMPLES=ON ..
```

### 选择内存分配器

```bash
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

## 代码规范

### 命名约定

- **函数**: `uvhttp_module_action` (如 `uvhttp_server_new`)
- **类型**: `uvhttp_name_t` (如 `uvhttp_server_t`)
- **常量**: `UVHTTP_UPPER_CASE` (如 `UVHTTP_MAX_HEADERS`)
- **宏**: `UVHTTP_UPPER_CASE` (如 `UVHTTP_MALLOC`)

### 代码风格

- **标准**: C11
- **缩进**: 4 个空格
- **大括号**: K&R 风格
- **行长度**: 最大 120 字符

### 注释规范

```c
/**
 * @brief 函数简要说明
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 */
uvhttp_error_t uvhttp_function(int param1, const char* param2);
```

## 内存管理

### 统一分配器

UVHTTP 提供统一的内存管理接口，通过编译期选择分配器类型。

#### 基本操作

```c
// 分配内存
void* ptr = uvhttp_alloc(size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// 重新分配
ptr = uvhttp_realloc(ptr, new_size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// 释放内存
uvhttp_free(ptr);

// 分配并初始化
ptr = uvhttp_calloc(count, size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}
```

#### 分配器类型

UVHTTP 支持两种分配器类型：

1. **系统分配器**（默认）
   - 稳定可靠，无额外依赖
   - 零抽象开销
   - 适合大多数场景

2. **mimalloc 分配器**
   - 高性能现代分配器
   - 内置小对象优化
   - 更好的多线程扩展性
   - 降低内存碎片

#### 编译配置

```bash
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

#### 性能特性

- **零运行时开销**: 所有函数都是内联函数
- **编译期优化**: 编译器可以完全优化
- **类型安全**: 编译期类型检查
- **可预测性**: 无动态分发

#### 最佳实践

1. **统一使用**: 始终使用 `uvhttp_alloc/uvhttp_free`，不要混用 `malloc/free`
2. **成对分配**: 每个分配都有对应的释放
3. **检查返回值**: 检查分配是否成功
4. **避免泄漏**: 确保所有路径都释放内存
5. **避免双重释放**: 释放后将指针设为 `NULL`

#### 完整示例

```c
#include "uvhttp_allocator.h"

void example_memory_usage(void) {
    // 分配内存
    char* buffer = uvhttp_alloc(1024);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }

    // 使用内存
    strcpy(buffer, "Hello, World!");

    // 重新分配
    buffer = uvhttp_realloc(buffer, 2048);
    if (!buffer) {
        fprintf(stderr, "Failed to reallocate memory\n");
        return;
    }

    // 释放内存
    uvhttp_free(buffer);
    buffer = NULL;  // 避免悬空指针
}
```

### 内存泄漏检测

使用 Valgrind 检测内存泄漏：

```bash
# 编译 Debug 版本
cmake -DENABLE_DEBUG=ON ..
make

# 运行 Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./dist/bin/helloworld
```

## 错误处理

### 错误码

所有错误码都是负数，`UVHTTP_OK (0)` 表示成功。

```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_IO = -3,
    // ... 更多错误码
} uvhttp_error_t;
```

### 错误处理模式

```c
// 基本错误处理
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "Description: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "Suggestion: %s\n", uvhttp_error_suggestion(result));
    return 1;
}

// 错误恢复
if (result != UVHTTP_OK && uvhttp_error_is_recoverable(result)) {
    // 尝试恢复操作
    result = uvhttp_server_listen(server, "0.0.0.0", 8081);
}
```

### 错误处理原则

1. **检查所有可能失败的函数调用**
2. **使用统一的错误类型**
3. **提供有意义的错误信息**
4. **支持错误恢复**

## 测试

### 运行测试

```bash
# 运行所有测试
./run_tests.sh

# 运行特定测试
./dist/bin/test_router_full_coverage

# 生成覆盖率报告
./run_tests.sh --detailed
```

### 编写测试

```c
#include <gtest/gtest.h>

TEST(UvhttpTest, BasicFunctionality) {
    // 测试代码
    EXPECT_EQ(result, UVHTTP_OK);
}
```

### 测试覆盖

- **单元测试**: 测试单个函数
- **集成测试**: 测试模块交互
- **性能测试**: 测试性能指标

## 性能优化

### 零拷贝优化

```c
// 使用 libuv 缓冲区
uv_buf_t buf = uv_buf_init(data, len);
uv_write(req, stream, &buf, 1, callback);

// 使用 sendfile
uvhttp_static_sendfile("/path/to/file", response);
```

### 缓存策略

```c
// 预热缓存
uvhttp_static_prewarm_cache(ctx, "/static/index.html");

// 使用 LRU 缓存
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_new(1024);
```

### 内存优化

```c
// 使用内联函数
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}

// 避免不必要的拷贝
const char* data = uvhttp_request_get_body(request, &len);
```

## 调试技巧

### 启用调试输出

```bash
cmake -DENABLE_DEBUG=ON ..
make
```

### 使用 GDB

```bash
gdb ./dist/bin/helloworld
(gdb) run
(gdb) backtrace
(gdb) print variable
```

### 日志输出

```c
#include "uvhttp_logging.h"

// 设置日志级别
uvhttp_log_set_level(UVHTTP_LOG_LEVEL_DEBUG);

// 输出日志
UVHTTP_LOG_DEBUG("Debug message: %s", message);
UVHTTP_LOG_INFO("Info message: %s", message);
UVHTTP_LOG_ERROR("Error message: %s", message);
```

## 提交代码

### 提交规范

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 类型

- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

### 示例

```
feat(server): add WebSocket support

Implement WebSocket protocol support for real-time
communication.

- Add WebSocket handshake handling
- Add frame parsing and generation
- Add connection management

Closes #123
```

## 代码审查

### 审查要点

- **内存管理**: 正确使用 `uvhttp_alloc/uvhttp_free`
- **错误处理**: 检查所有可能失败的函数
- **命名规范**: 遵循命名约定
- **代码风格**: 符合代码规范
- **性能**: 避免不必要的拷贝和分配
- **安全性**: 输入验证和边界检查

### 审查流程

1. 创建 Pull Request
2. 自动运行 CI/CD 测试
3. 代码审查
4. 修改并重新审查
5. 合并到主分支

## 常见问题

### Q: 如何选择内存分配器？

A: 
- **系统分配器**: 适合大多数场景，稳定可靠
- **mimalloc**: 适合高并发、多线程场景，性能更好

```bash
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### Q: 如何处理内存泄漏？

A: 
1. 使用 Valgrind 检测泄漏
2. 确保每个 `uvhttp_alloc` 都有对应的 `uvhttp_free`
3. 使用 RAII 模式管理资源

### Q: 如何优化性能？

A: 
1. 使用零拷贝技术
2. 启用缓存
3. 使用 mimalloc 分配器
4. 避免不必要的内存分配

### Q: 如何调试网络问题？

A: 
1. 启用调试日志
2. 使用 tcpdump 抓包
3. 使用 GDB 调试
4. 检查错误码和错误信息

## 参考资料

- [架构设计文档](../dev/ARCHITECTURE.md)
- [API 参考文档](../api/API_REFERENCE.md)
- [教程](../guide/TUTORIAL.md)
- [libuv 文档](https://docs.libuv.org/)
- [HTTP/1.1 规范](https://tools.ietf.org/html/rfc7230)

## 许可证

MIT License - 详见 LICENSE 文件