# 版本信息

UVHTTP 版本与兼容性说明。

## 当前版本

**版本**: 2.5.0
**发布日期**: 2026-03-15
**状态**: 稳定

## 版本历史

### 2.5.0 (2026-03-17)

**新功能**:
- 测试覆盖率提升，新增 172 个测试用例
- test_version_full_coverage.cpp (43 个) — 版本和构建信息 API
- test_error_complete_coverage.cpp (58 个) — 错误码和处理
- test_protocol_upgrade_api_coverage.cpp (26 个) — 协议升级 API
- test_connection_public_api_coverage.cpp (45 个) — 连接管理 API
- 优化 test_utils_full_coverage.cpp — 工具测试扩展

**改进**:
- 代码覆盖率 85.4% → 89.2%（+3.8%）
- 函数覆盖率 92.0% (7336/7974)
- 行覆盖率 89.2% (13386/15008)

**模块覆盖率**:
- uvhttp_version.c: 0.0% → 98.3%
- uvhttp_error.c: 31.7% → 98.8%
- uvhttp_protocol_upgrade.c: 39.6% → 73.6%
- uvhttp_connection.c: 42.6% → 42.8%

**高覆盖率模块** (≥95%):
- uvhttp_utils.c: 100.0%
- uvhttp_error.c: 98.8%
- uvhttp_version.c: 98.3%
- uvhttp_error_helpers.c: 95.9%

**测试**:
- 172 个新测试全部通过
- 公共 API 覆盖
- NULL 参数处理
- 错误条件和边界情况

**文档**:
- 更新性能基准
- API 文档增加覆盖率统计
- 更新测试覆盖率报告

### 2.4.1 (2026-02-13)

**新功能**:
- 4 个核心模块新增 99 个测试用例
- test_static_extended_coverage.cpp (25 个) — uvhttp_static.c
- test_router_extended_coverage.cpp (25 个) — uvhttp_router.c
- test_connection_extended_coverage.cpp (24 个) — uvhttp_connection.c
- test_request_extended_coverage.cpp (25 个) — uvhttp_request.c

**Bug 修复**:
- 修复连接生命周期清理中的内存泄漏（每个连接约 18KB）
- 修复 CMakeLists.txt 中未定义的 MOCK_TEST_FILES
- 修复 run_all_tests.sh 中 58 个硬编码路径

**改进**:
- 测试覆盖率：router (62.9%), connection (60.7%), request (~60%), static (62.5%)
- router 和 connection 超过 60% 覆盖率目标
- 58 个测试正确编译，mock 测试过滤正常

**文档**:
- 更新 CHANGELOG.md
- 更新 versions.md

### 2.4.0 (2026-02-12)

**新功能**:
- CMake 导出配置，简化库集成
- install(EXPORT) 替代 export()
- 导出目标添加 NAMESPACE uvhttp::
- find_dependency() 查找依赖
- pkg-config 支持 (uvhttp.pc.in)

**Bug 修复**:
- WebSocket 集成测试要求 BUILD_WITH_WEBSOCKET
- test_server_simple_api_coverage 要求 BUILD_WITH_WEBSOCKET
- 静态文件示例条件编译
- 覆盖率报告生成错误处理

**改进**:
- CI/CD 构建矩阵 TLS 重命名为 HTTPS
- CI/CD 错误处理和覆盖率报告
- CI/CD 构建矩阵验证：15/15 通过

**文档**:
- CMake 安装指南 (docs/INSTALL_CMAKE.md)
- CMake 导出配置文档
- 更新 CHANGELOG.md

### 2.2.2 (2026-02-02)

**主要变更**:
- 路由缓存优化：O(1) 路由查找
- 修复路由参数 bug
- 添加递归深度限制，防止栈溢出
- 生产代码中文注释翻译为英文

**性能改进**:
- 峰值吞吐量：21,991 RPS（目标 23,070 RPS 的 95.3%）
- 最低延迟：551 μs
- 最大吞吐量：23.02 MB/s

**Bug 修复**:
- match_route_node 路径参数丢失
- 递归深度限制
- 移除重复函数声明
- 代码风格

**文档**:
- 更新性能基准
- 添加 2.2.2 变更日志
- 更新 README.md

### 2.2.1 (2026-01-31)

**破坏性变更**:
- TLS 错误类型集成：所有 TLS API 返回 `uvhttp_error_t`
- 移除 `uvhttp_tls_error_t`

**Bug 修复**:
- TLS 错误类型一致性
- TLS 错误码

### 2.2.0 (2025-01-30)

**主要变更**:
- CI/CD 拆分为 32 位和 64 位工作流
- 文档国际化（英文和中文）
- 修复 32 位构建兼容性
- 更新验证函数

**改进**:
- 峰值吞吐量 31,883 RPS
- 错误处理改进
- 文档改进

**Bug 修复**:
- 32 位 WebSocket 移位溢出
- 32 位兼容性验证函数
- CI/CD 工作流

### 2.1.0 (2025-01-20)

**主要变更**:
- 移除全局变量
- libuv 数据指针模式
- 测试覆盖率

**新功能**:
- WebSocket
- 静态文件服务
- 限流
- 内存泄漏检测

**性能**:
- 大文件零拷贝
- LRU 缓存
- 连接池

### 2.0.0 (2025-01-10)

**主要变更**:
- 完全重写
- 新 API 设计
- 模块化架构

**破坏性变更**:
- 与 1.x 不兼容
- 函数重命名为 `uvhttp_module_action` 格式
- 新错误处理系统

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
- 函数名称已更改
- 新错误处理系统
- 不同初始化过程

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
uvhttp_server_t* server = NULL;
uvhttp_server_new(loop, &server);
uvhttp_router_t* router = NULL;
uvhttp_router_new(&router);
uvhttp_server_set_router(server, router);
```

### 从 2.0 升级到 2.1

**新功能**:
- WebSocket
- 静态文件服务
- 限流

**迁移步骤**:

无破坏性变更。新功能通过编译标志启用：

```bash
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

### 从 2.1 升级到 2.2

**破坏性变更**:

1. **TLS 错误类型集成** (2.2.1)
   - TLS API 返回 `uvhttp_error_t` 替代 `uvhttp_tls_error_t`
   - 错误码集成到统一错误系统

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
   - 路由缓存优化默认启用
   - 新增路由缓存统计

   **迁移**:
   ```c
   // 无需代码更改
   // 路由缓存自动启用
   // 禁用：定义 UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 0
   ```

**新功能**:
- O(1) 路由缓存
- 路径参数处理改进
- 错误消息增强
- 性能监控

**Bug 修复**:
- 嵌套路由路径参数丢失
- 路由匹配栈溢出
- 错误处理内存泄漏

**性能改进**:
- 峰值吞吐量：21,991 RPS（从 19,776 RPS 提升）
- 最低延迟：551 μs（从 352 μs 提升）
- 路由缓存减少开销 50%+

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

### 从 2.4 升级到 2.5

**破坏性变更**:

1. **构造函数改为输出参数风格**
   - `uvhttp_server_new` 与 `uvhttp_router_new` 接收输出参数，返回 `uvhttp_error_t`

   ```c
   // 旧版本 (2.4.x)
   uvhttp_server_t* server = uvhttp_server_new(loop);
   uvhttp_router_t* router = uvhttp_router_new();
   server->router = router;

   // 新版本 (2.5.x)
   uvhttp_server_t* server = NULL;
   uvhttp_error_t r = uvhttp_server_new(loop, &server);
   if (r != UVHTTP_OK) { /* 处理错误 */ }

   uvhttp_router_t* router = NULL;
   uvhttp_router_new(&router);
   uvhttp_server_set_router(server, router);
   ```

2. **请求处理函数签名**
   - 处理函数接收 request 和 response，返回 `int`。无 `uvhttp_response_new()`

   ```c
   // 旧版本 (2.4.x)
   void hello_handler(uvhttp_request_t* req) {
       uvhttp_response_t* res = uvhttp_response_new(req);
       uvhttp_response_set_body(res, "Hello");
       uvhttp_response_send(res);
   }

   // 新版本 (2.5.x)
   int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
       uvhttp_response_set_status(res, 200);
       uvhttp_response_set_body(res, "Hello");
       return uvhttp_response_send(res);
   }
   ```

3. **内存安全验证**
   - 全部测试通过 AddressSanitizer（零泄漏、零 use-after-free、零溢出）
   - UndefinedBehaviorSanitizer 验证
   - 完整修复列表见 [更新日志](../guide/CHANGELOG.md)

## 发布流程

1. 在 `develop` 分支开发
2. 稳定后合并到 `main`
3. 创建发布分支
4. 打标签发布
5. 部署到生产环境

## 支持策略

### LTS（长期支持）

- **持续时间**: 6 个月
- **更新**: 安全修复
- **当前 LTS**: 2.5.x

### 稳定版

- **持续时间**: 3 个月
- **更新**: Bug 修复和安全修复
- **当前稳定版**: 2.5.x

### 开发版

- **持续时间**: 直到下一个稳定版
- **更新**: 所有更改，包括破坏性变更
- **当前开发版**: 2.6.x（develop 分支）

## 获取帮助

- **文档**: [完整文档](/)
- **问题**: [GitHub Issues](https://github.com/adam-ikari/uvhttp/issues)
- **讨论**: [GitHub Discussions](https://github.com/adam-ikari/uvhttp/discussions)

## 变更日志

详细变更日志见 [CHANGELOG.md](CHANGELOG.md)
