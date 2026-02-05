# 全局变量重构计划

## 概述

UVHTTP 项目中存在 4 个静态全局变量，违反了项目的"避免全局变量"原则。本文档记录了将这些全局变量重构为使用 loop->data 模式的计划。

## 当前全局变量

### 1. `g_tls_initialized` (uvhttp_tls.c)
```c
static int g_tls_initialized = 0;
```
**用途**: 跟踪 TLS 模块是否已初始化，避免重复初始化
**影响**: TLS 连接创建
**优先级**: 中

### 2. `g_drbg_initialized` (uvhttp_websocket_native.c)
```c
static int g_drbg_initialized = 0;
```
**用途**: 跟踪 DRBG (Deterministic Random Bit Generator) 是否已初始化
**影响**: WebSocket 连接创建
**优先级**: 中

### 3. `error_stats` (uvhttp_error.c)
```c
static uvhttp_error_stats_t error_stats = {0};
```
**用途**: 记录错误统计信息
**影响**: 错误监控和调试
**优先级**: 低

### 4. `g_config_callback` (uvhttp_config.c)
```c
static uvhttp_config_change_callback_t g_config_callback = NULL;
```
**用途**: 配置变更回调函数
**影响**: 配置热重载和监控
**优先级**: 中

### 5. `g_current_config` (uvhttp_config.c)
```c
static uvhttp_config_t* g_current_config = NULL;
```
**用途**: 当前活动的配置
**影响**: 所有配置操作
**优先级**: 高

## 重构策略

### 阶段 1: 扩展服务器上下文结构体

```c
typedef struct {
    /* 现有字段 */
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    
    /* TLS 状态 */
    int tls_initialized;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    /* WebSocket 状态 */
    int drbg_initialized;
    
    /* 错误统计 */
    uvhttp_error_stats_t error_stats;
    
    /* 配置管理 */
    uvhttp_config_t* current_config;
    uvhttp_config_change_callback_t config_callback;
} uvhttp_app_context_t;
```

### 阶段 2: 修改 API 签名

#### TLS 模块
```c
// 旧 API
uvhttp_tls_error_t uvhttp_tls_init(void);

// 新 API
uvhttp_tls_error_t uvhttp_tls_init(uvhttp_app_context_t* ctx);
```

#### WebSocket 模块
```c
// 旧 API
uvhttp_error_t uvhttp_websocket_init(void);

// 新 API
uvhttp_error_t uvhttp_websocket_init(uvhttp_app_context_t* ctx);
```

#### 错误模块
```c
// 旧 API
void uvhttp_record_error(uvhttp_error_t error, const char* context);

// 新 API
void uvhttp_record_error(uvhttp_app_context_t* ctx, uvhttp_error_t error, const char* context);
```

#### 配置模块
```c
// 旧 API
int uvhttp_config_monitor_changes(uvhttp_config_change_callback_t callback);
int uvhttp_config_reload(void);

// 新 API
int uvhttp_config_monitor_changes(uvhttp_app_context_t* ctx, uvhttp_config_change_callback_t callback);
int uvhttp_config_reload(uvhttp_app_context_t* ctx);
```

### 阶段 3: 更新所有调用点

需要更新以下模块中的所有调用：
- `uvhttp_server.c` - 服务器初始化和 TLS 初始化
- `uvhttp_websocket_native.c` - WebSocket 连接创建
- `uvhttp_error.c` - 错误记录
- `uvhttp_config.c` - 配置管理
- `uvhttp_tls.c` - TLS 初始化

### 阶段 4: 更新测试

所有使用这些全局变量的测试都需要更新以传递上下文。

### 阶段 5: 更新文档

更新以下文档：
- API_REFERENCE.md
- DEVELOPER_GUIDE.md
- LIBUV_DATA_POINTER.md
- 示例代码

## 实施步骤

1. **创建分支**: `refactor/global-variable-elimination`
2. **扩展上下文结构体**: 添加所有全局变量字段
3. **修改 API**: 逐步修改每个模块的 API 签名
4. **更新调用点**: 更新所有使用旧 API 的地方
5. **更新测试**: 确保所有测试通过
6. **更新文档**: 更新 API 文档和示例
7. **性能测试**: 确保性能没有下降
8. **代码审查**: 提交 PR 进行审查
9. **合并**: 合并到主分支

## 风险评估

| 风险 | 级别 | 缓解措施 |
|------|------|---------|
| API 破坏性变更 | 🔴 高 | 主版本升级，提供迁移指南 |
| 性能下降 | 🟡 中 | 性能测试，优化热点 |
| 测试失败 | 🟡 中 | 逐步更新测试，确保覆盖率 |
| 文档不一致 | 🟢 低 | 同步更新文档 |

## 预期收益

1. **符合项目规范**: 消除全局变量，使用 loop->data 模式
2. **多实例支持**: 支持在同一个进程中运行多个服务器实例
3. **更好的测试**: 单元测试更容易隔离和管理
4. **云原生友好**: 更适合容器化和微服务架构

## 时间估算

- 阶段 1-2: 4-6 小时
- 阶段 3-4: 8-12 小时
- 阶段 5-6: 4-6 小时
- 阶段 7-9: 4-6 小时

**总计**: 20-30 小时

## 版本计划

建议在 **v2.0.0** 版本中实施此重构，因为这是一个破坏性的 API 变更。

## 参考资料

- [LIBUV_DATA_POINTER.md](LIBUV_DATA_POINTER.md) - libuv 数据指针模式指南
- [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) - 贡献者指南
- [API_REFERENCE.md](API_REFERENCE.md) - API 参考