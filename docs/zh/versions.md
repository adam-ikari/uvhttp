# 版本信息

本文档提供 UVHTTP 版本信息及其兼容性说明。

## 当前版本

**版本**: 2.4.1  
**发布日期**: 2026-02-13  
**状态**: 稳定

## 版本历史

### 2.4.1 (2026-02-13)

**新功能**:
- 扩展测试覆盖率，为 4 个核心模块添加 99 个新测试用例
- test_static_extended_coverage.cpp (25 个测试) 用于 uvhttp_static.c
- test_router_extended_coverage.cpp (25 个测试) 用于 uvhttp_router.c
- test_connection_extended_coverage.cpp (24 个测试) 用于 uvhttp_connection.c
- test_request_extended_coverage.cpp (25 个测试) 用于 uvhttp_request.c

**Bug 修复**:
- 修复连接生命周期清理中的关键内存泄漏（每个连接约 18KB）
- 修复 CMakeLists.txt 中未定义的 MOCK_TEST_FILES
- 修复 run_all_tests.sh 中的 58 个硬编码路径以提高可移植性

**改进**:
- 测试覆盖率提升：router (62.9%), connection (60.7%), request (~60%), static (62.5%)
- 2 个模块（router 和 connection）超过 60% 覆盖率目标
- 所有 58 个测试现在都能正确编译，具有正确的 mock 测试过滤

**文档**:
- 更新 CHANGELOG.md 添加 v2.5.0 发布信息
- 更新 versions.md 添加 v2.5.0 版本信息

### 2.4.0 (2026-02-12)

**新功能**:
- CMake 导出配置，简化库集成
- 使用 install(EXPORT) 替代 export()
- 添加 NAMESPACE uvhttp:: 到导出目标
- 依赖项通过 find_dependency() 在 uvhttp-config.in.cmake 中查找
- 添加 pkg-config 支持 (uvhttp.pc.in)
- 简化库用户的集成流程

**Bug 修复**:
- 修复 WebSocket 集成测试要求 BUILD_WITH_WEBSOCKET
- 修复 test_server_simple_api_coverage 要求 BUILD_WITH_WEBSOCKET
- 修复静态文件示例条件编译
- 修复覆盖率报告生成错误处理

**改进**:
- CI/CD 构建矩阵中 TLS 重命名为 HTTPS
- 改进 CI/CD 错误处理和覆盖率报告生成
- CI/CD 构建矩阵验证：15/15 通过

**文档**:
- 添加 CMake 安装指南 (docs/INSTALL_CMAKE.md)
- 添加 CMake 导出配置文档
- 更新 CHANGELOG.md 添加 v2.4.0 发布信息

### 2.2.2 (2026-02-02)

**主要变更**:
- 路由缓存优化：O(1) 路由查找，支持热路径缓存
- 修复路由参数 bug：路径参数现在正确保留
- 添加递归深度限制以防止栈溢出
- 将生产代码中的所有中文注释翻译为英文

**性能改进**:
- 峰值吞吐量：21,991 RPS（达到 23,070 RPS 目标的 95.3%）
- 最低延迟：551 μs
- 最大吞吐量：23.02 MB/s

**Bug 修复**:
- 修复 `match_route_node` 函数中的路径参数丢失问题
- 添加递归深度限制以防止栈溢出
- 移除重复的函数声明
- 修复代码风格不一致问题

**文档更新**:
- 更新性能基准测试数据
- 添加 2.2.2 变更日志条目
- 使用最新指标更新 README.md

### 2.2.1 (2026-01-31)

**破坏性变更**:
- TLS 错误类型集成：所有 TLS API 现在返回 `uvhttp_error_t`
- 移除 `uvhttp_tls_error_t` 类型

**Bug 修复**:
- 修复 TLS 错误类型一致性问题
- 添加全面的 TLS 错误码

### 2.2.0 (2025-01-30)

**主要变更**:
- 将 CI/CD 拆分为单独的 32 位和 64 位工作流
- 添加文档国际化支持（英文和中文）
- 修复 32 位构建兼容性问题
- 更新验证函数以提高安全性

**改进**:
- 性能优化：峰值吞吐量可达 31,883 RPS
- 更好的错误处理和报告
- 改进的文档

**Bug 修复**:
- 修复 32 位 WebSocket 实现中的移位溢出
- 修复 32 位兼容性的验证函数
- 修复 CI/CD 工作流问题

### 2.1.0 (2025-01-20)

**主要变更**:
- 重构以移除全局变量
- 实现 libuv 数据指针模式
- 添加全面的测试覆盖率

**新功能**:
- WebSocket 支持
- 静态文件服务
- 限流
- 内存泄漏检测

**性能**:
- 大文件零拷贝优化
- LRU 缓存实现
- 连接池

### 2.0.0 (2025-01-10)

**主要变更**:
- 从头开始完全重写
- 新的 API 设计
- 模块化架构

**破坏性变更**:
- 新 API 与 1.x 不兼容
- 所有函数重命名为 `uvhttp_module_action` 格式
- 新的错误处理系统

## 兼容性

### 平台支持

| 平台 | 版本 | 状态 |
|------|------|------|
| Linux x86_64 | 2.2.0+ | ✅ 稳定 |
| Linux i386 | 2.2.0+ | ✅ 稳定 |
| macOS x86_64 | 2.2.0+ | ✅ 稳定 |
| macOS ARM64 | 2.2.0+ | ✅ 稳定 |
| Windows x86_64 | 2.2.0+ | ⚠️ 实验性 |

### 编译器支持

| 编译器 | 版本 | 状态 |
|--------|------|------|
| GCC | 4.8+ | ✅ 稳定 |
| Clang | 3.4+ | ✅ 稳定 |
| MSVC | 2019+ | ⚠️ 实验性 |

### 依赖版本

| 依赖 | 版本 | 状态 |
|------|------|------|
| libuv | 1.44.0+ | ✅ 必需 |
| llhttp | 8.1.0+ | ✅ 必需 |
| mbedtls | 3.0.0+ | ✅ 可选（TLS） |
| mimalloc | 2.0.0+ | ✅ 可选（分配器） |
| cjson | 1.7.0+ | ✅ 可选（JSON） |

## 升级指南

### 从 1.x 升级到 2.0

**破坏性变更**:
- 所有函数名称已更改
- 新的错误处理系统
- 不同的初始化过程

**迁移步骤**:

1. 更新函数名称：
```c
// 旧版本
server_new(loop);
router_add_route(router, "/api", handler);

// 新版本
uvhttp_server_new(loop);
uvhttp_router_add_route(router, "/api", handler);
```

2. 更新错误处理：
```c
// 旧版本
if (server == NULL) {
    // 处理错误
}

// 新版本
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
}
```

3. 更新初始化：
```c
// 旧版本
uvhttp_server_t* server = server_new(loop);

// 新版本
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;
```

### 从 2.0 升级到 2.1

**新功能**:
- WebSocket 支持
- 静态文件服务
- 限流

**迁移步骤**:

无破坏性变更。新功能通过编译标志可选启用：

```bash
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

### 从 2.1 升级到 2.2

**破坏性变更**:

1. **TLS 错误类型集成** (2.2.1)
   - 所有 TLS API 函数现在返回 `uvhttp_error_t` 而不是 `uvhttp_tls_error_t`
   - 错误码已集成到统一错误系统中

   **迁移**:
   ```c
   // 旧版本 (2.1.x)
   uvhttp_tls_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_TLS_OK) { /* 处理错误 */ }
   
   // 新版本 (2.2.x)
   uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_OK) { /* 处理错误 */ }
   ```

2. **路由缓存 API 变更** (2.2.2)
   - 路由缓存优化现在默认启用
   - 提供新的路由缓存统计信息

   **迁移**:
   ```c
   // 无需代码更改
   // 路由缓存自动启用
   // 要禁用：定义 UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 0
   ```

**新功能**:
- O(1) 查找的路由缓存优化
- 改进的路径参数处理
- 增强的错误消息
- 更好的性能监控

**Bug 修复**:
- 修复嵌套路由中的路径参数丢失问题
- 修复路由匹配中潜在的栈溢出
- 修复错误处理中的内存泄漏

**性能改进**:
- 峰值吞吐量：21,991 RPS（从 19,776 RPS 提升）
- 最低延迟：551 μs（从 352 μs 提升）
- 路由缓存减少路由匹配开销 50%+

## 发布计划

### 开发分支

- **分支**: `develop`
- **状态**: 活跃开发
- **稳定性**: 可能包含破坏性变更

### 主分支

- **分支**: `main`
- **状态**: 稳定发布候选
- **稳定性**: 已测试且稳定

### 发布分支

- **分支**: `release`
- **状态**: 生产就绪
- **稳定性**: 完全测试且有文档

## 发布流程

1. 在 `develop` 分支开发
2. 稳定后合并到 `main`
3. 为版本创建发布分支
4. 打标签发布
5. 部署到生产环境

## 支持策略

### LTS（长期支持）

- **持续时间**: 6 个月
- **更新**: 仅安全修复
- **当前 LTS**: 2.2.x

### 稳定版

- **持续时间**: 3 个月
- **更新**: Bug 修复和安全修复
- **当前稳定版**: 2.2.x

### 开发版

- **持续时间**: 直到下一个稳定版发布
- **更新**: 所有更改，包括破坏性变更
- **当前开发版**: 2.3.x（develop 分支）

## 获取帮助

- **文档**: [完整文档](/)
- **问题**: [GitHub Issues](https://github.com/adam-ikari/uvhttp/issues)
- **讨论**: [GitHub Discussions](https://github.com/adam-ikari/uvhttp/discussions)

## 变更日志

详细变更日志，请查看 [CHANGELOG.md](../CHANGELOG.md)