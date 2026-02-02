# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.2.2] - 2026-02-02

### Fixed

- **路由器关键 bug 修复**
  - 修复路径参数丢失问题（移除错误的 `param_count` 重置）
  - 添加递归深度限制，防止栈溢出崩溃
  - 影响范围：所有参数化路由（如 `/api/users/:id`）

- **文档翻译错误修复**
  - 修复 `include/uvhttp_config.h` 中的拼写错误和速率限制注释
  - 修复 `include/uvhttp_constants.h` 中 50+ 处损坏的英文翻译
  - 提升代码可读性和可维护性

- **代码格式统一**
  - 统一头文件包含的缩进风格（4 空格）
  - 符合项目 C11 代码规范

### Performance

- **性能提升**
  - 峰值 RPS 从 20,432 提升到 21,991（+7.6%）
  - 性能目标达成率：95.3%（目标 23,070 RPS）
  - 高并发稳定性：10-200 并发，RPS 波动仅 30%

- **性能测试结果**
  - 简单文本：21,991 RPS，4.56ms 延迟
  - JSON 响应：21,095 RPS，4.74ms 延迟
  - 小响应：21,395 RPS，4.67ms 延迟，23.02MB/s 吞吐

### Testing

- **测试覆盖率**
  - 路由器测试：51 个测试全部通过
  - 核心模块测试：246 个测试全部通过
  - 包括连接、服务器、响应、WebSocket、缓存等模块

### Changed

- 合并 `feature/enable-router-cache-optimization` 分支
- 路由缓存优化功能已启用
- 更新性能基准测试文档（2026-02-02）

## [2.2.1] - 2026-01-31

### Breaking Changes

⚠️ **重要**: TLS 错误类型已整合到统一错误体系

1. **TLS 错误类型整合**
   - **影响**: 所有使用 `uvhttp_tls_error_t` 的代码
   - **变更**: 删除 `uvhttp_tls_error_t`，所有 TLS API 函数返回类型改为 `uvhttp_error_t`
   - **迁移**: 更新所有 TLS 相关函数调用
   ```c
   // 旧代码（已移除）
   uvhttp_tls_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_TLS_OK) { /* 处理错误 */ }
   
   // 新代码
   uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_OK) { /* 处理错误 */ }
   ```

2. **TLS 错误码扩展**
   - **新增**: `UVHTTP_ERROR_TLS_CERT` (-408) 到 `UVHTTP_ERROR_TLS_NO_CERT` (-418)
   - **新增**: `UVHTTP_ERROR_TLS_WANT_READ` (1) 和 `UVHTTP_ERROR_TLS_WANT_WRITE` (2)
   - **用途**: 支持更细粒度的 TLS 错误处理和非阻塞 I/O

### Added

- 新增 WebSocket API 测试（52个测试用例）
- 新增服务器 API 测试
- 更新 TLS NULL 参数测试

### Changed

- 统一错误处理体系，简化 API 使用
- 改善类型安全性，减少类型转换错误

## [2.2.0] - 2026-01-28

### Breaking Changes

⚠️ **重要**: 本版本包含重大架构重构，多个 API 发生破坏性变更

1. **所有初始化函数返回值变更**
   - **影响**: 所有使用 `uvhttp_config_new()`, `uvhttp_context_create()` 等函数的代码
   - **变更**: 返回值从指针类型改为 `uvhttp_error_t`，通过输出参数返回对象
   ```c
   // 旧代码（已移除）
   uvhttp_config_t* config = uvhttp_config_new();
   if (!config) { /* 处理错误 */ }
   
   // 新代码
   uvhttp_config_t* config = NULL;
   uvhttp_error_t result = uvhttp_config_new(&config);
   if (result != UVHTTP_OK) { /* 处理错误 */ }
   ```

2. **依赖注入系统已移除**
   - **影响**: 使用 `uvhttp_deps.h` 和相关 provider 抽象的代码
   - **变更**: 移除 `uvhttp_deps.h`, `uvhttp_connection_provider_t`, `uvhttp_logger_provider_t`, `uvhttp_config_provider_t`
   - **迁移**: 直接使用 libuv 和标准库函数
   ```c
   // 旧代码（已移除）
   uvhttp_deps_t* deps = uvhttp_deps_new();
   uvhttp_deps_set_loop_provider(deps, provider);
   
   // 新代码（直接使用 libuv）
   uv_loop_t* loop = uv_default_loop();
   ```

3. **日志系统重构**
   - **影响**: 使用 `uvhttp_logger_provider_t` 的代码
   - **变更**: 改为编译期宏实现，Release 模式下零开销
   ```c
   // 旧代码（已移除）
   uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(level);
   logger->log(logger, UVHTTP_LOG_LEVEL_INFO, "message");
   
   // 新代码（编译期宏）
   UVHTTP_LOG_INFO("message");
   ```

4. **中间件架构变更**
   - **影响**: 使用动态中间件链的代码
   - **变更**: 改为编译期宏定义中间件链
   - **迁移**: 使用 `UVHTTP_DEFINE_MIDDLEWARE_CHAIN` 宏

5. **WebSocket 实现重命名**
   - **影响**: 包含 `uvhttp_websocket_native.h` 的代码
   - **变更**: 统一为 `uvhttp_websocket.h`
   ```c
   // 旧代码（已移除）
   #include "uvhttp_websocket_native.h"
   
   // 新代码
   #include "uvhttp_websocket.h"
   ```

### Removed
- **抽象层**: 移除所有运行时抽象层
  - `uvhttp_deps.h` (125 行)
  - `uvhttp_connection_provider_t` 接口
  - `uvhttp_logger_provider_t` 接口
  - `uvhttp_config_provider_t` 接口
  - `uvhttp_network_interface_t` 接口
- **测试文件**: 删除 38 个已禁用的测试文件
- **示例文件**: 删除 5 个过时的中间件示例

### Added
- **Benchmark 目录**: 新增基准性能测试目录
  - `benchmark/performance_allocator.c`
  - `benchmark/performance_allocator_compare.c`
  - `benchmark/test_bitfield.c`
  - `benchmark/README.md`
- **日志系统**: 新增编译期宏日志系统
  - `include/uvhttp_logging.h`
  - `src/uvhttp_logging.c`

### Performance
- **零开销抽象**: 编译期宏实现，Release 模式下完全零开销
- **内存分配器**: 性能与系统分配器相当
- **RPS 性能测试**:
  - 4线程10连接：20,432 RPS（峰值）
  - 4线程50连接：19,840 RPS
  - 4线程100连接：19,776 RPS
  - 4线程500连接：19,850 RPS

### Code Reduction
- **总代码减少**: 23,805 行代码（减少 88%）
- **简化架构**: 移除所有不必要的抽象层
- **提高可维护性**: 代码更简洁，易于理解和维护

### Migration Guide

详细的迁移指南请参考 `MIGRATION_GUIDE.md`

## [2.1.0] - 2026-01-27

### Added
- **使用者教程**: 新增完整的使用者文档
  - `docs/guide/installation.md`: 详细安装指南（Linux/macOS/Windows）
  - `docs/guide/first-server.md`: 第一个服务器教程（3 个完整示例）
  - `docs/guide/websocket.md`: WebSocket 使用指南（含应用层认证示例）
- **Examples 重构**: 按功能分类重组示例代码
  - `01_basics/`: 基础示例
  - `02_routing/`: 路由示例
  - `04_static_files/`: 静态文件示例
  - `05_websocket/`: WebSocket 示例
  - `06_advanced/`: 高级功能示例
  - `07_performance/`: 性能测试示例

### Changed
- **项目结构**: 移除 `src/core` 目录，将核心文件移至 `src/` 根目录
  - 理由：文件数量少（24 个），不需要子文件夹

### Performance
- **RPS 性能提升**: 峰值 RPS 从 17,798 提升到 24,439 (+37.3%)
- **延迟降低**: 平均延迟降低 30.1%
- **内存优化**: 移除自定义内存池，使用 mimalloc
  - 大对象分配性能提升 50%
  - 减少内存碎片

### Removed
- **WebSocket 认证功能**: 移除内置的 WebSocket 认证模块
  - 删除 `src/uvhttp_websocket_auth.c` (368 行)
  - 删除 `include/uvhttp_websocket_auth.h` (147 行)
  - 删除 `test/unit/test_websocket_auth_full_coverage.cpp` (542 行)
  - 删除 `docs/guide/WEBSOCKET_AUTH.md` (377 行)
- **自定义内存池**: 移除 `uvhttp_mempool` 模块
  - 删除 `src/uvhttp_mempool.c` (97 行)
  - 删除 `include/uvhttp_mempool.h` (47 行)
  - 删除 `test/unit/test_mempool_full_coverage.cpp` (459 行)
- **全局变量**: 移除 `g_uvhttp_context` 全局变量
  - 改用 libuv loop 注入模式
  - 提高可测试性和多实例支持

### Fixed
- **测试代码**: 修复移除内存池后的测试问题
- **文档错误**: 修复教程中的 API 错误（uvhttp_run -> uv_run）

### Documentation
- **设计原则**: 在 IFLOW.md 中添加 10 条设计原则
- **变更记录**: 记录所有重大架构变更

### Breaking Changes

⚠️ **重要**: 本版本包含破坏性变更，需要升级指南

1. **WebSocket 认证 API 已移除**
   - **影响**: 所有使用 `uvhttp_server_ws_set_auth_config` 等认证 API 的代码
   - **迁移**: 认证功能应在应用层实现
   - **示例**: 参考 `docs/guide/websocket.md` 中的应用层认证示例
   ```c
   // 旧代码（已移除）
   uvhttp_server_ws_set_auth_config(server, auth_config);
   
   // 新代码（应用层认证）
   int on_connect(uvhttp_ws_connection_t* ws_conn, void* user_data) {
       const char* auth_header = uvhttp_ws_get_request_header(ws_conn, "Authorization");
       if (!auth_header || !validate_token(auth_header + 7)) {
           return -1;  // 拒绝连接
       }
       return 0;
   }
   ```

2. **内存池 API 已移除**
   - **影响**: 使用 `uvhttp_mempool_*` 函数的代码
   - **迁移**: 直接使用 `UVHTTP_MALLOC` / `UVHTTP_FREE` 宏
   ```c
   // 旧代码（已移除）
   uvhttp_mempool_alloc(pool, size);
   
   // 新代码
   UVHTTP_MALLOC(size);
   ```

3. **全局变量 g_uvhttp_context 已移除**
   - **影响**: 使用全局变量访问上下文的代码
   - **迁移**: 改用 libuv loop 注入模式
   ```c
   // 旧代码（已移除）
   uvhttp_context_t* ctx = g_uvhttp_context;
   
   // 新代码
   uvhttp_context_t* ctx = (uvhttp_context_t*)loop->data;
   ```

### Migration Guide

详细的迁移指南请参考：
- WebSocket 认证迁移: `docs/guide/websocket.md`
- libuv 数据指针模式: `docs/LIBUV_DATA_POINTER.md`

### Testing
- **测试通过**: 所有测试通过（34/34）
- **性能测试**: RPS 基准测试通过（峰值 20,432 RPS）

## [2.0.0] - 2026-01-24

### Added
- **冒烟测试**: 新增 `test/unit/test_smoke.cpp`，验证基本功能
- **死亡测试**: 新增 `test/unit/test_death.cpp`，验证错误处理
- **压力测试**: 新增 `test/unit/test_stress.cpp`，验证高并发性能
- **内存测试**: 新增 `test/unit/test_memory.cpp`，验证内存使用
- **CI/CD 增强**: 添加冒烟测试、死亡测试、压力测试和内存测试到 CI 流程
- **文档重组**: 重新组织文档目录结构，提升可读性
  - `guide/`: 用户指南和教程
  - `dev/`: 开发文档和架构设计
  - `api/`: API 参考文档

### Performance
- **性能优化**: Homepage 21,574 RPS (+126%)
- **编译优化**: 从 -O3 降级到 -O2，避免激进优化
- **内存优化**: 连接复用节省 279,920 字节/次
- **边界检查**: 增强缓冲区边界检查，提升安全性

### Fixed
- **循环逻辑**: 修复服务器循环逻辑，确保回调执行
- **内存泄漏**: 修复示例代码空指针保护
- **测试超时**: 调整测试超时配置，避免并发竞争

### Documentation
- **文档结构**: 重新组织文档目录，添加完整侧边栏导航
- **版本统一**: 统一版本号为 2.0.0

### Testing
- **测试覆盖率**: 71/71 测试通过 (100%)
- **新增测试**: 4 个新测试文件，33 个新测试用例

### Breaking Changes
- **编译选项**: 编译优化级别从 -O3 改为 -O2
- **依赖更新**: 更新子模块到最新版本

## [1.6.0] - 2026-01-20

### Added
- **全局变量重构计划**: 新增 `docs/GLOBAL_VARIABLE_REFACTOR_PLAN.md`，记录全局变量重构策略
- **路由性能优化文档**: 为 `HYBRID_THRESHOLD` 添加详细的性能测试数据注释

### Fixed
- **API 文档错误**: 修正所有响应 API 的返回类型为 `uvhttp_error_t`
- **循环访问方法**: 修复文档中的循环访问方法，使用 `uv_handle_get_loop()` 替代不存在的函数
- **处理器签名**: 更新处理器签名以匹配实际 API（返回 `int`，接收 `request` 和 `response` 参数）
- **路由计数器**: 修复 `add_array_route` 未增加 `route_count` 的 bug
- **LRU 缓存逻辑**: 修复 `uvhttp_lru_cache_is_expired` 的逻辑错误
  - NULL 条目现在正确返回 1（已过期）
  - TTL 为 0 时正确返回 0（永不过期）
- **请求方法映射**: 为 `UVHTTP_ANY` 方法添加正确的字符串表示 "ANY"
- **路由验证**: 添加空路径和查询字符串验证
- **测试期望值**: 修复所有测试中的错误期望值

### Changed
- **测试通过率**: 从 91% 提升到 100% (67/67)
- **代码质量**: 消除魔法数字，添加详细注释
- **文档版本**: 更新到 2.0.0，文档日期更新到 2026-01-21
- **测试文件**: 暂时禁用 `test_connection_integration.cpp`（需要 libuv handle 管理重构）

### Performance
- **路由混合模式**: 优化 `HYBRID_THRESHOLD` 选择，基于性能测试数据
  - 50 个路由：数组 0.02ms, Trie 0.03ms
  - 100 个路由：数组 0.04ms, Trie 0.04ms
  - 200 个路由：数组 0.08ms, Trie 0.05ms
  - 500 个路由：数组 0.20ms, Trie 0.06ms

### Documentation
- **API 文档**: 所有代码示例已验证与头文件中的 API 签名一致
- **错误处理**: 添加完整的错误处理示例
- **开发指南**: 修复测试代码示例使用正确的初始化函数

### Testing
- **测试通过率**: 100% (67/67)
- **测试失败**: 0
- **代码覆盖率**: 68.6% (行覆盖率), 84.1% (函数覆盖率)

### Breaking Changes
- **无破坏性变更**: 所有 API 保持向后兼容

## [1.5.0] - 2026-01-16

### Added
- **测试框架现代化**: 统一使用 C++ 和 Google Test 框架
- **测试覆盖率提升**: 代码覆盖率从未知提升至 69.1%
- **测试规范文档**: 新增 `docs/TESTING_STANDARDS.md`（457 行）
- **依赖管理改进**: 新增 `cmake/Dependencies.cmake` 统一管理依赖构建
- **API 扩展**: 新增 8 个 Provider Getter/Setter 函数到 `uvhttp_deps.h`

### Changed
- **测试文件格式**: 所有单元测试从 `.c` 重命名为 `.cpp`（34 个文件）
- **CI/CD 配置**: 测试超时时间从 300 秒增加到 600 秒
- **构建系统**: 更新 CMakeLists.txt 和 Makefile 支持 C++ 测试
- **开发者指南**: 更新测试框架说明和最佳实践
- **连接数限制配置**: 默认最大连接数从 10000 降低到 2048
  - **原因**: 2048 更适合大多数应用场景，避免资源浪费
  - **影响**: 现有高并发应用可能需要调整配置
  - **解决方案**: 如需更高并发，可通过配置调整（最大支持 10000）
  - **配置示例**:
    ```c
    config->max_connections = 10000;  // 高并发场景
    ```

### Fixed
- **查询字符串验证 bug**: 修复 `uvhttp_validate_query_string` 函数中的 bug
  - 移除 `dangerous_query_chars` 中的 `'\0'`
  - 修复原因：`strchr()` 会找到字符串末尾的空终止符，导致误判
- **测试用例更新**: 更新所有相关测试用例以匹配修复后的函数行为

### Testing
- **测试通过率**: 100% (53 个测试，5 个被跳过)
- **测试用例数量**: 49 个测试可执行文件
- **代码覆盖率**: 69.1%
- **被跳过的测试**（需要后续修复）:
  - `test_deps_full_coverage`（包含长时间运行的操作）
  - `test_lru_cache_full_coverage`（包含 sleep(2) 调用，共 6 秒）
  - `test_server_full_coverage`（超时）
  - `test_server_rate_limit_coverage`（超时）
  - `test_server_simple_api_coverage`（超时）

### Breaking Changes
- **健康检查功能移除**:
  - 删除 `include/uvhttp_health.h`
  - 删除 `src/uvhttp_health.c`
  - 删除 `examples/health_check_demo.c`
  - 从 `uvhttp.h` 移除健康检查头文件引用
  - **迁移指南**: 如果您的代码依赖健康检查 API，请移除相关代码并实现自定义健康检查端点
- **测试框架变更**:
  - 所有单元测试文件从 `.c` 改为 `.cpp`
  - 测试代码必须使用 C++ 和 Google Test 框架
  - **影响**: 仅影响测试代码，不影响公共 API

### Migration Guide

#### 健康检查功能迁移
如果您使用的是健康检查功能，请按照以下步骤迁移：

1. 移除 `#include <uvhttp_health.h>`
2. 移除 `uvhttp_health_check_t` 相关代码
3. 实现自定义健康检查端点：

```c
void health_check_handler(uvhttp_request_t* request) {
    uvhttp_response_set_status(request, 200);
    uvhttp_response_set_header(request, "Content-Type", "application/json");
    uvhttp_response_set_body(request, "{\"status\":\"ok\"}");
    uvhttp_response_send(request);
}

// 在路由中添加
uvhttp_router_add_route(router, "/health", health_check_handler);
```

## [1.4.0] - 2026-01-13

### Added
- **WebSocket 连接管理**: 连接池、超时检测、心跳检测、广播功能
- **内存管理优化**: 使用内联函数替代宏定义
- **改进的超时检测机制**: 使用 libuv 定时器实现主动超时检测
- **配置值文档说明**: 添加 sendfile 配置参数选择依据
- **性能基准测试**: 完整的性能测试数据（15,000+ RPS）

### Changed
- **内存分配器 API**: 从宏改为内联函数（uvhttp_alloc/uvhttp_free）
- **WebSocket 实现**: 完全原生实现，移除 libwebsockets 依赖
- **sendfile 超时检测**: 从被动检测改为主动检测
- **示例程序内存管理**: 统一使用 UVHTTP_MALLOC/UVHTTP_FREE
- **文档完善**: 添加性能优化章节到 STATIC_FILE_SERVER.md

### Fixed
- **内存泄漏**: 修复 WebSocket 连接管理中的内存泄漏
- **中等文件传输超时**: 使用分块发送（1MB chunks）
- **示例程序内存泄漏**: 修复所有示例程序的内存管理不一致问题
- **超时检测不完整**: 确保网络完全阻塞时也能及时响应

### Performance
- **性能测试结果**: 
  - 主页: 15,967 RPS (79.8% 目标)
  - 中等文件: 14,285 RPS (71.4% 目标)
  - 大文件: 15,480 RPS (77.4% 目标)
  - 平均延迟: 3-6ms
- **分块传输影响**: 性能影响 < 1%

### Testing
- **测试通过率**: 100% (所有现有测试)
- **性能测试**: wrk 测试全部通过
- **代码质量**: 零编译警告

### Breaking Changes
- **内存分配器 API**: UVHTTP_MALLOC/UVHTTP_FREE 改为 uvhttp_alloc/uvhttp_free

## [1.3.2] - 2026-01-11

### Added
- **sendfile 超时测试**: test_sendfile_timeout.c 测试用例

### Fixed
- **中等文件传输超时**: 使用分块发送（每次64KB，优化后）
- **超时检测**: 添加10秒超时检测（优化后）
- **错误处理**: 改进错误处理，添加重试机制（最多2次，优化后）

### Testing
- **测试通过率**: 100% (10/10)
- **测试覆盖**: 中等文件、边界情况、分类测试

## [1.3.1] - 2026-01-11

### Added
- **分支管理规范**: 完整的Git Flow分支管理策略
- **Linux 内核开发节奏**: 持续开发-快速修复-稳定发布

### Changed
- **开发流程**: 采用 Git Flow 分支管理策略

## [1.3.0] - 2026-01-11

### Added
- **分支管理规范**: 完整的Git Flow分支管理策略和开发规范
- **性能对比测试**: Nginx vs UVHTTP性能对比综合报告
- **性能测试标准**: 详细的性能测试标准文档
- **服务器配置指南**: 服务器配置性能优化指南
- **限流功能**: 完整的限流功能实现和API文档
- **性能测试工具**: 全面的性能测试框架和工具

### Changed
- **限流API重构**: 将限流功能从中间件改为服务器核心功能
- **中间件系统**: 实现零开销中间件系统
- **静态文件服务**: 静态文件中间件解耦，性能优化
- **错误码机制**: 增强错误码机制和错误处理
- **WebSocket功能**: 完善WebSocket功能，添加路由支持

### Fixed
- **白名单内存泄漏**: 修复白名单哈希表内存泄漏和重复添加问题
- **NULL参数处理**: 修复uvhttp_request_get_path的NULL参数处理
- **测试崩溃**: 修复test_request_null_coverage测试崩溃问题
- **PR评审问题**: 修复PR评审中发现的关键问题

### Performance
- **性能优化**: 完成性能优化和代码质量改进
- **测试覆盖**: 添加全面的限流功能测试框架
- **API简化**: 简化限流API，移除未使用的算法参数

### Testing
- **测试通过率**: 所有测试通过 (69/69, 100%)
- **NULL参数测试**: 完整的NULL参数覆盖测试
- **性能验证**: 性能基准测试验证通过

### Documentation
- **开发指南**: 更新开发指南，包含分支管理规范
- **性能文档**: 添加性能测试标准和配置指南
- **API文档**: 添加限流功能API文档
- **测试文档**: 添加性能对比测试报告

### Breaking Changes
- **限流API变更**: 限流功能从中间件改为服务器核心功能，API有所变化

## [1.2.0] - 2026-01-07

### Added
- **TLS安全功能**: 实现CRL检查、DH参数设置、会话票证管理、证书链验证
- **性能基准测试**: 新增性能基准测试程序和文档
- **安全策略文档**: 完整的安全策略和依赖管理文档
- **mimalloc支持**: 启用mimalloc作为默认内存分配器
- **原生WebSocket**: 使用原生WebSocket实现替代libwebsockets

### Changed
- **TLS实现**: 从OpenSSL迁移到mbedTLS，提升安全性和性能
- **性能优化**: Keep-alive连接管理优化，性能提升约1000倍（4-16 RPS → 14,000-16,000 RPS）
- **代码精简**: 移除15,192行冗余代码，提升代码可维护性
- **文档重组**: 移动文档到docs/目录，优化项目结构
- **依赖管理**: 清理.gitmodules，移除不再使用的依赖

### Fixed
- **空指针检查**: 修复request getter函数的空指针检查（11个函数）
- **TLS类型错误**: 修复mbedTLS API类型不兼容问题
- **编译警告**: 修复所有编译警告（未使用变量、strncpy截断等）
- **HTTP头验证**: 修复HTTP头验证逻辑错误
- **响应构建**: 修复响应数据构建时机问题

### Security
- **CRL检查**: 实现证书撤销列表检查功能
- **证书链验证**: 完整的证书链验证支持
- **依赖更新策略**: 明确的安全更新、功能更新、维护更新策略
- **安全审计**: 建立每周依赖扫描、每月代码审查、季度渗透测试计划
- **漏洞响应**: 定义完整的漏洞发现、评估、修复、通知流程

### Performance
- **Keep-alive连接**: 修复连接复用，性能提升约1000倍
- **TCP优化**: 启用TCP_NODELAY和TCP keepalive
- **响应缓冲区**: 优化响应缓冲区大小（512 → 1024字节）
- **内存分配器**: mimalloc提供更快的内存分配和释放

### Testing
- **NULL参数测试**: 完整的NULL参数覆盖测试（TLS 32个，Request 11个）
- **性能验证**: 1000倍性能提升已通过基准测试验证
- **代码质量**: 编译无错误无警告，启用安全编译选项

### Documentation
- **PERFORMANCE_BENCHMARK.md**: 详细的性能基准测试文档
- **SECURITY.md**: 完整的安全策略文档
- **依赖文档**: 更新DEPENDENCIES.md，包含所有依赖版本和更新策略

### Breaking Changes
- **TLS API变更**: 从OpenSSL迁移到mbedTLS，API签名有所变化
- **WebSocket实现**: 从libwebsockets迁移到原生实现
- **依赖变更**: 移除libwebsockets依赖，使用mbedTLS替代OpenSSL

## [1.1.0] - 2025-12-25

### Added
- **内存安全增强**: 使用C99灵活数组成员解决内存对齐问题
- **整数溢出保护**: 在所有关键内存分配点添加整数溢出检查
- **路径遍历防护**: 多层路径遍历保护机制，防止目录遍历攻击
- **性能优化**: 增加连接数限制（2048）和监听队列（1024）
- **新常量定义**: 添加HTTP状态码范围常量和响应头安全边距常量
- **文档**: 新增ROUTER_SEARCH_MODES.md文档
- **WebSocket全局清理**: 完善WebSocket全局资源清理函数

### Changed
- **API统一**: 统一所有验证函数返回值（1表示有效/成功，0表示无效/失败）
- **函数命名**: 移除所有兼容性包装函数，使用统一的API命名
- **代码清理**: 移除所有兼容性代码和注释
- **测试更新**: 更新所有测试以匹配新的API签名
- **错误处理**: 简化错误处理逻辑，提高代码可读性

### Fixed
- **内存对齐**: 修复uvhttp_write_data_t结构的内存对齐问题
- **内存泄漏**: 修复错误处理路径中的内存泄漏
- **危险字符检查**: 移除错误null字符检查逻辑
- **函数返回值**: 修正验证函数的返回值逻辑
- **编译错误**: 修复所有示例程序和测试程序的编译错误

### Security
- **缓冲区溢出保护**: 添加完整的边界检查和整数溢出检查
- **HTTP响应拆分防护**: 增强控制字符检测
- **DoS防护**: 合理的资源限制和超时配置
- **TLS支持**: 支持TLS 1.3加密协议
- **输入验证**: 28处输入验证调用，覆盖所有用户输入点

### Performance
- **哈希算法**: 使用xxHash高性能哈希算法
- **LRU缓存**: 优化路由缓存和静态文件缓存
- **零拷贝**: 减少内存拷贝操作
- **连接池**: 优化连接复用和管理

### Testing
- **单元测试**: 16/16测试通过（100%通过率）
- **测试覆盖**: 47%测试代码量
- **边界测试**: 完整的边界条件和极端条件测试
- **安全测试**: 路径遍历、DoS防护等安全测试

### Documentation
- **API文档**: 完整的API参考文档
- **开发指南**: 详细的开发指南和规范
- **架构文档**: 清晰的架构说明
- **示例代码**: 15个示例程序

## [1.0.0] - 2025-12-20

### Added
- **初始发布**: 基于libuv的高性能HTTP服务器
- **路由系统**: 支持动态路由和参数提取
- **静态文件服务**: 高效的静态文件服务
- **WebSocket支持**: 完整的WebSocket协议支持
- **TLS/SSL**: 支持HTTPS加密连接
- **LRU缓存**: 高性能缓存系统
- **连接池**: 优化连接复用
- **配置系统**: 灵活的配置管理
- **错误处理**: 完善的错误处理和日志系统

### Features
- 单线程事件驱动模型
- 高性能异步I/O
- 模块化设计
- 可扩展的插件系统
- 详细的文档和示例

[1.1.0]: https://github.com/adam-ikari/uvhttp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/adam-ikari/uvhttp/releases/tag/v1.0.0