# 版本信息

UVHTTP 版本与兼容性说明。

## 当前版本

**版本**: 2.5.1
**发布日期**: 2026-07-25
**状态**: 稳定

## 版本历史

详见 [更新日志](./guide/CHANGELOG.md) 获取完整的版本历史。
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
