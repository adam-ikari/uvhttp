# UVHTTP 构建配置矩阵

本文档详细说明了 UVHTTP 支持的各种构建配置组合及其用途。

## 概述

UVHTTP 支持通过 CMake 配置选项灵活地组合不同的功能模块，以满足不同的使用场景需求。

本文档主要介绍常用的核心配置选项。高级配置选项（如缓冲区大小、连接限制、缓存参数等）请参考 [高级编译选项](ADVANCED_BUILD_OPTIONS.md)。

## 功能选项

这些选项控制编译到库中的功能模块。

### BUILD_WITH_WEBSOCKET
- **类型**: BOOL
- **默认值**: ON
- **说明**: 启用 WebSocket 支持（同时支持 ws:// 和 wss://）
- **影响**:
  - 编译 `src/uvhttp_websocket.c`
  - 链接 WebSocket 相关依赖
  - 提供 WebSocket API
  - 同时支持 ws://（WebSocket）和 wss://（WebSocket Secure）
- **依赖**: mbedtls 库（需要 base64、SHA1、随机数生成和 TLS 加密功能）
- **注意**:
  - ws:// 和 wss:// 都需要 TLS 库（mbedtls）
  - ws:// 使用 mbedtls 进行帧编码和安全功能
  - wss:// 使用 mbedtls 提供 TLS 加密层
  - 只要 BUILD_WITH_HTTPS 或 BUILD_WITH_WEBSOCKET 有一个为 ON，就会编译 mbedtls

### BUILD_WITH_HTTPS
- **类型**: BOOL
- **默认值**: ON
- **说明**: 启用 HTTPS 支持
- **影响**:
  - 编译 `src/uvhttp_tls.c`
  - 链接 mbedtls 库
  - 提供 TLS 上下文和加密功能
  - 支持 HTTPS 和 WSS（WebSocket Secure）
  - 设置 UVHTTP_FEATURE_TLS=1 宏
- **依赖**: mbedtls 库
- **注意**:
  - 只要 BUILD_WITH_HTTPS 或 BUILD_WITH_WEBSOCKET 有一个为 ON，就会编译 mbedtls
  - 如果 BUILD_WITH_HTTPS 和 BUILD_WITH_WEBSOCKET 都为 OFF，则不编译 mbedtls

### BUILD_WITH_MIMALLOC
- **类型**: BOOL
- **默认值**: 基于 `UVHTTP_ALLOCATOR_TYPE`（系统/自定义为 OFF，mimalloc 为 ON）
- **说明**: 使用 mimalloc 内存分配器
- **影响**:
  - 链接 mimalloc 库
  - 提供更快的内存分配性能（提升 30-50%）
  - 减少内存碎片
- **依赖**: 无
- **注意**:
  - 当 `UVHTTP_ALLOCATOR_TYPE=1` 时自动启用
  - 可以手动启用，不受分配器类型限制
  - 对于 `UVHTTP_ALLOCATOR_TYPE=0`（系统）和 `UVHTTP_ALLOCATOR_TYPE=2`（自定义）默认禁用

### ENABLE_DEBUG
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用调试模式
- **影响**:
  - 禁用编译器优化
  - 启用调试符号
  - 启用日志输出（即使 UVHTTP_FEATURE_LOGGING=0）
- **依赖**: 无

### ENABLE_COVERAGE
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用代码覆盖率
- **影响**:
  - 添加覆盖率编译标志
  - 生成覆盖率报告
- **依赖**: `ENABLE_DEBUG=ON`（推荐）

### BUILD_EXAMPLES
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 编译示例程序
- **影响**:
  - 编译 examples/ 目录下的所有示例
  - 生成可执行文件到 build/dist/bin/
- **依赖**: 无

## 构建模式选项

这些选项控制构建模式（调试、发布、覆盖率）。

### BUILD_BENCHMARKS
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 编译性能测试程序
- **影响**:
  - 编译 benchmark/ 目录下的所有性能测试程序
  - 生成可执行文件到 build/dist/bin/
- **依赖**: 无

### ENABLE_LTO
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用链接时优化（LTO）
- **影响**:
  - 跨编译单元优化
  - 可能提升性能 5-15%
  - 增加编译时间
- **依赖**: `CMAKE_BUILD_TYPE=Release`（推荐）
- **冲突**: `ENABLE_DEBUG=ON`、`ENABLE_ASAN=ON`、`ENABLE_TSAN=ON`

### ENABLE_PGO
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用配置文件引导优化（PGO）
- **影响**:
  - 基于实际运行数据优化
  - 可能提升性能 10-20%
  - 需要两步编译过程
- **依赖**: `CMAKE_BUILD_TYPE=Release`
- **冲突**: `ENABLE_DEBUG=ON`

### ENABLE_FASTER_MATH
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用快速数学优化
- **影响**:
  - 使用 -ffast-math 编译选项
  - 可能提升浮点运算性能
  - 可能影响数值精度
- **依赖**: 无

## 调试工具

这些选项启用运行时错误检测工具（Sanitizers）。

### ENABLE_ASAN
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用 AddressSanitizer（内存错误检测）
- **影响**:
  - 检测内存越界、释放后使用等错误
  - 性能下降 50-70%
  - 内存占用增加 2-3 倍
- **依赖**: `ENABLE_DEBUG=ON`（推荐）
- **冲突**: `ENABLE_LTO=ON`、`ENABLE_TSAN=ON`

### ENABLE_UBSAN
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用 UndefinedBehaviorSanitizer（未定义行为检测）
- **影响**:
  - 检测未定义行为
  - 性能下降 20-30%
- **依赖**: `ENABLE_DEBUG=ON`（推荐）
- **冲突**: `ENABLE_LTO=ON`

### ENABLE_TSAN
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用 ThreadSanitizer（线程安全检测）
- **影响**:
  - 检测数据竞争
  - 性能下降 50-70%
  - 内存占用增加 5-10 倍
- **依赖**: `ENABLE_DEBUG=ON`（推荐）
- **冲突**: `ENABLE_LTO=ON`、`ENABLE_ASAN=ON`

### ENABLE_VALGRIND
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用 Valgrind 支持
- **影响**:
  - 添加 Valgrind 兼容性代码
  - 便于内存泄漏检测
- **依赖**: 无

### ENABLE_DEV_MODE
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用开发模式
- **影响**:
  - 启用日志系统（UVHTTP_FEATURE_LOGGING=1）
  - 添加开发调试辅助
  - 生产环境应禁用以获得最佳性能
- **依赖**: 无

## 日志选项
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用 Valgrind 支持
- **影响**:
  - 添加 Valgrind 兼容性代码
  - 便于内存泄漏检测
- **依赖**: 无

这些选项控制日志详细程度。

### ENABLE_LOG_DEBUG
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用调试日志级别
- **影响**:
  - 输出详细调试信息
  - 可能影响性能
- **依赖**: 无

### ENABLE_LOG_TRACE
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用跟踪日志级别
- **影响**:
  - 输出最详细的跟踪信息
  - 显著影响性能
- **依赖**: 无

### ENABLE_LOG_PERFORMANCE
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 启用性能日志
- **影响**:
  - 记录性能指标
  - 轻微性能影响
- **依赖**: 无

## 安全选项

这些选项启用安全加固特性。

### ENABLE_HARDENING
- **类型**: BOOL
- **默认值**: ON
- **说明**: 启用安全加固
- **影响**:
  - 栈保护、RELRO 等安全特性
  - 轻微性能影响
- **依赖**: 无

### ENABLE_STACK_PROTECTION
- **类型**: BOOL
- **默认值**: ON
- **说明**: 启用栈保护
- **影响**:
  - 防止栈溢出攻击
  - 轻微性能影响
- **依赖**: 无

### ENABLE_FORTIFY_SOURCE
- **类型**: BOOL
- **默认值**: ON
- **说明**: 启用 _FORTIFY_SOURCE
- **影响**:
  - 增强缓冲区溢出检测
  - 轻微性能影响
- **依赖**: 无

## 高级配置选项

高级配置选项（如缓冲区大小、连接限制、缓存参数等）已移至单独的文档：

**[高级编译选项](ADVANCED_BUILD_OPTIONS.md)**

高级配置选项主要用于：
- 性能调优
- 内存受限环境
- 特定场景优化
- 资源约束环境

大多数用户不需要修改这些选项，默认值已经针对通用场景进行了优化。

## 配置依赖关系图

```
BUILD_WITH_WEBSOCKET (ON)
    └─> mbedtls 库 [必需 - 提供 base64、SHA1、随机数生成和加密]

BUILD_WITH_HTTPS (ON)
    └─> mbedtls 库 [必需 - 为 HTTPS 和 WSS 提供 TLS]

mbedtls 库
    └─> 当 BUILD_WITH_HTTPS=ON 或 BUILD_WITH_WEBSOCKET=ON 时编译

BUILD_WITH_MIMALLOC (ON)
    └─> 无依赖

ENABLE_DEBUG (OFF)
    └─> 无依赖

ENABLE_COVERAGE (OFF)
    └─> ENABLE_DEBUG=ON [推荐]

ENABLE_LTO (OFF)
    └─> CMAKE_BUILD_TYPE=Release [推荐]
    └─> !ENABLE_DEBUG [冲突]
    └─> !ENABLE_ASAN [冲突]
    └─> !ENABLE_TSAN [冲突]

ENABLE_PGO (OFF)
    └─> CMAKE_BUILD_TYPE=Release [必需]
    └─> !ENABLE_DEBUG [冲突]

ENABLE_ASAN (OFF)
    └─> ENABLE_DEBUG=ON [推荐]
    └─> !ENABLE_LTO [冲突]
    └─> !ENABLE_TSAN [冲突]

ENABLE_UBSAN (OFF)
    └─> ENABLE_DEBUG=ON [推荐]
    └─> !ENABLE_LTO [冲突]

ENABLE_TSAN (OFF)
    └─> ENABLE_DEBUG=ON [推荐]
    └─> !ENABLE_LTO [冲突]
    └─> !ENABLE_ASAN [冲突]

ENABLE_LOG_DEBUG (OFF)
    └─> 无依赖

ENABLE_LOG_TRACE (OFF)
    └─> 无依赖

ENABLE_LOG_PERFORMANCE (OFF)
    └─> 无依赖

ENABLE_HARDENING (ON)
    └─> 无依赖

ENABLE_STACK_PROTECTION (ON)
    └─> 无依赖

ENABLE_FORTIFY_SOURCE (ON)
    └─> 无依赖

ENABLE_FASTER_MATH (OFF)
    └─> 无依赖

ENABLE_VALGRIND (OFF)
    └─> 无依赖

ENABLE_DEV_MODE (OFF)
    └─> 无依赖
```

## 配置组合建议

### 开发环境
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_LOG_DEBUG=ON \
      -DENABLE_LOG_TRACE=ON \
      -DBUILD_EXAMPLES=ON
```

### 测试环境
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_ASAN=ON \
      -DENABLE_UBSAN=ON \
      -DENABLE_COVERAGE=ON
```

### 生产环境（安全加固）
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_HARDENING=ON \
      -DENABLE_STACK_PROTECTION=ON \
      -DENABLE_FORTIFY_SOURCE=ON
```

### 生产环境（性能优化）
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_LTO=ON
```

### 性能测试环境
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DBUILD_BENCHMARKS=ON \
      -DENABLE_LOG_PERFORMANCE=ON
```

### 最小化部署
```bash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```

## 注意事项

### 依赖关系
1. **WebSocket 依赖 HTTPS**：启用 `BUILD_WITH_WEBSOCKET` 时必须启用 `BUILD_WITH_HTTPS`
2. **HTTPS 依赖 mbedtls**：启用 `BUILD_WITH_HTTPS` 需要链接 mbedtls 库
3. **WebSocket 依赖 mbedtls**：启用 `BUILD_WITH_WEBSOCKET` 需要 mbedtls 提供 base64、SHA1 和随机数生成功能
4. **Sanitizer 互斥**：`ENABLE_ASAN`、`ENABLE_TSAN` 不能同时启用
5. **LTO 与 Debug 互斥**：`ENABLE_LTO` 与 `ENABLE_DEBUG` 不能同时启用
6. **Coverage 需要 Debug**：`ENABLE_COVERAGE` 推荐配合 `ENABLE_DEBUG=ON` 使用

### 性能影响
- **Sanitizer**: 性能下降 50-70%
- **LTO**: 编译时间增加 50-100%，性能提升 5-15%
- **mimalloc**: 内存分配性能提升 30-50%
- **WebSocket**: 增加 ~50KB 二进制体积（不含 mbedtls）
- **HTTPS (mbedtls)**: 增加 ~200KB 二进制体积
- **WebSocket + HTTPS (mbedtls)**: 增加 ~250KB 二进制体积
- **性能测试程序**: 增加 ~500KB 二进制体积
- **Debug 模式**: 性能下降 50-80%

### BUILD_BENCHMARKS
- **类型**: BOOL
- **默认值**: OFF
- **说明**: 编译性能测试程序
- **影响**:
  - 编译 benchmark/ 目录下的所有性能测试程序
  - 生成可执行文件到 build/dist/bin/
  - 包含以下测试程序：
    - `benchmark_rps` - RPS 性能测试服务器
    - `benchmark_rps_many_routes` - 大量路由场景测试
    - `benchmark_rps_150_routes` - 150 路由场景测试
    - `benchmark_latency` - 延迟测试
    - `benchmark_connection` - 连接性能测试
    - `benchmark_memory` - 内存使用测试
    - `benchmark_comprehensive` - 综合性能测试
    - `benchmark_file_transfer` - 文件传输测试
    - `benchmark_router` - 路由器性能测试
    - `benchmark_router_simple` - 简单路由器测试
    - `benchmark_router_comparison` - 路由器对比测试
    - `benchmark_router_simple_comparison` - 简单路由器对比测试
    - `benchmark_router_minimal` - 最小路由器测试
    - `benchmark_database_simulation` - 数据库访问模拟测试
    - `benchmark_file_upload` - 文件上传测试

## 内存分配器配置

### UVHTTP_ALLOCATOR_TYPE
- **类型**: STRING
- **默认值**: 0
- **说明**: 内存分配器类型选择
- **选项**:
  - `0`: 系统分配器 (malloc/free)
  - `1`: mimalloc 分配器（自动启用 `BUILD_WITH_MIMALLOC=ON`）
  - `2`: 自定义分配器（应用层实现）
- **影响**:
  - 系统分配器：标准性能，无依赖
  - mimalloc：内存分配速度快 30-50%，减少碎片
  - 自定义：完全控制内存管理
- **注意**:
  - 选项 `1` 会自动启用 `BUILD_WITH_MIMALLOC=ON` 以方便使用
  - 选项 `2` 需要应用层实现自定义分配器函数
  - 自定义分配器实现详情请参考 [高级编译选项](ADVANCED_BUILD_OPTIONS.md)

### BUILD_WITH_MIMALLOC
- **类型**: BOOL
- **默认值**: 基于 `UVHTTP_ALLOCATOR_TYPE`（系统/自定义为 OFF，mimalloc 为 ON）
- **说明**: 使用 mimalloc 内存分配器
- **影响**:
  - 链接 mimalloc 库
  - 提供更快的内存分配性能（提升 30-50%）
  - 减少内存碎片
- **依赖**: 无
- **注意**:
  - 当 `UVHTTP_ALLOCATOR_TYPE=1` 时自动启用
  - 可以手动启用，不受分配器类型限制
  - 对于 `UVHTTP_ALLOCATOR_TYPE=0`（系统）和 `UVHTTP_ALLOCATOR_TYPE=2`（自定义）默认禁用

## 构建类型

### CMAKE_BUILD_TYPE
- **默认值**: Release
- **可选值**:
  - `Release`: 优化版本，无调试符号
  - `Debug`: 调试版本，无优化
  - `RelWithDebInfo`: 优化版本，带调试符号
  - `MinSizeRel`: 最小化体积版本

## 推荐配置矩阵

### 1. 最小配置（无可选功能）
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 最小化部署，仅核心 HTTP 功能
- **体积**: 最小
- **性能**: 基础性能

### 2. 完整功能（所有可选功能）
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 生产环境，完整功能
- **体积**: 中等
- **性能**: 最佳性能

### 3. Debug 模式
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 开发和调试
- **体积**: 较大
- **性能**: 无优化，便于调试

### 4. 覆盖率模式
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=ON \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 测试覆盖率分析
- **体积**: 最大
- **性能**: 无优化，带覆盖率检测

### 5. 系统分配器
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 使用系统内存分配器（默认）
- **体积**: 中等
- **性能**: 标准 malloc/free 性能

### 6. Mimalloc 分配器
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 使用 mimalloc 内存分配器
- **体积**: 中等
- **性能**: 更快的内存分配（提升 30-50%）

**注意**: 更多内存分配器配置选项（自定义分配器等）请参考 [高级编译选项](ADVANCED_BUILD_OPTIONS.md)。

### 7. 仅 WebSocket
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: WebSocket 应用，需要 HTTPS 支持
- **体积**: 较小
- **性能**: 良好

### 8. 仅 HTTPS
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: HTTPS 应用，无需 WebSocket
- **体积**: 较小
- **性能**: 良好

### 9. WebSocket + HTTPS
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: WebSocket Secure (WSS) 应用
- **体积**: 中等
- **性能**: 良好

### 10. 示例程序
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=ON
```
- **用途**: 学习和测试
- **体积**: 较大（包含示例程序）
- **性能**: 良好

### 12. 32 位兼容性检查
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DCMAKE_C_FLAGS=-m32 \
      -DCMAKE_CXX_FLAGS=-m32
```
- **用途**: 32 位系统兼容性测试
- **注意**: 需要系统支持 32 位编译

### 13. 静态分析准备
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic" \
      -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
```
- **用途**: 静态代码分析
- **编译器警告**: 启用所有警告

### 13. 最小化构建
```bash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 最小化二进制体积
- **体积**: 最小
- **性能**: 良好（优化体积）

### 14. 性能测试程序
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=OFF \
      -DBUILD_BENCHMARKS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 编译所有性能测试程序
- **体积**: 较大（包含所有 benchmark 程序）
- **性能**: 良好（Release 模式）
- **输出**: build/dist/bin/ 目录下生成所有 benchmark 可执行文件

### 15. 性能测试 + 示例程序
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=OFF \
      -DBUILD_BENCHMARKS=ON \
      -DBUILD_EXAMPLES=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF
```
- **用途**: 完整的开发和测试环境
- **体积**: 最大（包含示例和 benchmark）
- **性能**: 良好（Release 模式）

### 16. LTO 优化
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_LTO=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 链接时优化，提升性能
- **体积**: 较小
- **性能**: 最佳（跨编译单元优化）
- **编译时间**: 增加 50-100%

### 17. AddressSanitizer
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_ASAN=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 内存错误检测
- **体积**: 较大
- **性能**: 下降 50-70%
- **用途**: 开发和调试

### 18. ThreadSanitizer
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_TSAN=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 线程安全检测
- **体积**: 很大
- **性能**: 下降 50-70%
- **用途**: 并发调试

### 19. 调试日志
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_LOG_DEBUG=ON \
      -DENABLE_LOG_TRACE=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 详细日志输出
- **体积**: 较大
- **性能**: 下降 10-20%
- **用途**: 问题诊断

### 20. 安全加固
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_HARDENING=ON \
      -DENABLE_STACK_PROTECTION=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **用途**: 生产环境安全加固
- **体积**: 中等
- **性能**: 轻微影响
- **安全**: 栈保护、RELRO 等

## 配置验证

使用提供的测试脚本验证所有配置组合：

```bash
./test_cmake_configs.sh
```

该脚本会测试上述前 14 种配置组合，并报告通过/失败状态。

### 编译性能测试程序

编译所有性能测试程序：

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON ..
make -j$(nproc)
```

编译特定性能测试程序：

```bash
make benchmark_rps
make benchmark_latency
make benchmark_comprehensive
```

### 运行性能测试

运行 RPS 测试：

```bash
./build/dist/bin/benchmark_rps 18081
```

运行综合性能测试：

```bash
./build/dist/bin/benchmark_comprehensive 18082
```

使用 wrk 进行压力测试：

```bash
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
```

## 注意事项

### TLS 禁用限制
当 `BUILD_WITH_HTTPS=OFF` 时，源代码中的 TLS 相关代码需要用 `#ifdef UVHTTP_TLS_ENABLED` 包裹。当前版本中，以下文件包含未保护的 TLS 代码：

- `src/uvhttp_connection.c` - TLS 握手和清理函数
- `src/uvhttp_server.c` - TLS 上下文管理
- `src/uvhttp_context.c` - TLS 清理函数
- `src/uvhttp_websocket.c` - TLS 相关的 WebSocket 功能

**建议**: 如果需要禁用 TLS，需要先修改这些源文件，添加条件编译指令。

### 性能影响
- **mimalloc**: 内存分配性能提升 30-50%
- **WebSocket**: 增加 ~50KB 二进制体积
- **HTTPS**: 增加 ~200KB 二进制体积
- **WebSocket + HTTPS**: 增加 ~250KB 二进制体积
- **性能测试程序**: 增加 ~500KB 二进制体积
- **Debug 模式**: 性能下降 50-80%
- **LTO**: 性能提升 5-15%，编译时间增加 50-100%
- **ASAN/TSAN**: 性能下降 50-70%，内存占用增加 2-10 倍
- **性能测试程序**: 增加 ~500KB 二进制体积（所有 benchmark）

### 编译时间
- **最小配置**: ~30 秒
- **完整功能**: ~60 秒
- **覆盖率模式**: ~90 秒
- **性能测试程序**: ~120 秒
- **性能测试程序**: ~120 秒（包含所有 benchmark）

## 相关文档

- [高级编译选项](ADVANCED_BUILD_OPTIONS.md)

- [开发者指南](docs/guide/DEVELOPER_GUIDE.md)

- [构建模式详解](docs/zh/dev/BUILD_MODES.md)

- [CMake 目标链接指南](docs/dev/CMAKE_TARGET_LINKING_GUIDE.md)

- [性能基准测试](docs/dev/PERFORMANCE_BENCHMARK.md)

- [性能测试指南](benchmark/README.md)

- [性能测试标准](docs/dev/PERFORMANCE_TESTING_STANDARD.md)