# UVHTTP 示例程序

本目录包含 UVHTTP 的各种示例程序，按功能分类组织，帮助应用开发者快速学习和使用 UVHTTP。

## 目录结构

```
examples/
├── 01_basics/           # 基础示例
├── 02_routing/          # 路由示例
├── 03_middleware/       # 中间件示例
├── 04_static_files/     # 静态文件服务示例
├── 05_websocket/        # WebSocket 示例
├── 06_advanced/         # 高级功能示例
└── README.md            # 本文件
```

## 01_basics - 基础示例

最简单的 HTTP 服务器示例，适合初学者入门。

| 文件 | 描述 |
|------|------|
| `01_hello_world.c` | 最简单的 Hello World 服务器 |
| `helloworld.c` | 完整的 Hello World 示例 |
| `helloworld_complete.c` | 带完整错误处理的 Hello World |
| `quick_api_demo.c` | 快速 API 演示 |
| `simple_api_demo.c` | 简单 API 演示 |

**快速开始**:
```bash
cd build
make helloworld
./dist/bin/helloworld
```

## 02_routing - 路由示例

演示如何使用路由系统处理不同的 URL 和 HTTP 方法。

| 文件 | 描述 |
|------|------|
| `01_simple_routing.c` | 简单路由示例 |
| `02_method_routing.c` | HTTP 方法路由（GET、POST、PUT、DELETE） |

**快速开始**:
```bash
cd build
make simple_routing
./dist/bin/simple_routing
```

## 03_middleware - 中间件示例

演示如何使用中间件系统扩展服务器功能。

| 文件 | 描述 |
|------|------|
| `middleware_compile_time_demo.c` | 编译时中间件示例 |
| `middleware_chain_demo.c` | 中间件链示例 |
| `rate_limit_demo.c` | 限流中间件示例 |
| `test_middleware.c` | 中间件测试 |

**快速开始**:
```bash
cd build
make middleware_chain_demo
./dist/bin/middleware_chain_demo
```

## 04_static_files - 静态文件服务示例

演示如何提供静态文件服务，包括缓存优化。

| 文件 | 描述 |
|------|------|
| `simple_static_test.c` | 简单静态文件测试 |
| `static_file_server.c` | 静态文件服务器 |
| `cache_test_server.c` | 缓存测试服务器 |
| `advanced_static_server.c` | 高级静态文件服务器（带统计） |

**快速开始**:
```bash
cd build
make static_file_server
./dist/bin/static_file_server -d ../public -p 8080
```

## 05_websocket - WebSocket 示例

演示如何使用 WebSocket 功能实现实时通信。

| 文件 | 描述 |
|------|------|
| `websocket_echo_server.c` | WebSocket 回显服务器 |
| `websocket_test_server.c` | WebSocket 测试服务器 |
| `test_ws_connection_management.c` | WebSocket 连接管理测试 |

**快速开始**:
```bash
cd build
make websocket_echo_server
./dist/bin/websocket_echo_server
```

## 06_advanced - 高级功能示例

演示高级功能和最佳实践。

| 文件 | 描述 |
|------|------|
| `api_demo.c` | REST API 演示 |
| `json_api_demo.c` | JSON API 演示 |
| `config_demo.c` | 配置管理演示 |
| `simple_config.c` | 简单配置示例 |
| `context_injection.c` | 上下文注入示例 |
| `unified_response_demo.c` | 统一响应处理示例 |
| `app_advanced_memory.c` | 高级内存管理示例 |
| `hierarchical_allocator_example.c` | 分层分配器示例 |

**快速开始**:
```bash
cd build
make api_demo
./dist/bin/api_demo
```

## 编译示例

所有示例程序都可以通过以下方式编译：

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目（启用示例程序）
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON ..

# 编译所有示例
make

# 编译特定示例
make helloworld
make simple_routing
make middleware_chain_demo
make static_file_server
make websocket_echo_server
make api_demo
```

## 运行示例

编译完成后，示例程序位于 `build/dist/bin/` 目录：

```bash
# 运行示例
./build/dist/bin/helloworld
./build/dist/bin/simple_routing
./build/dist/bin/middleware_chain_demo
./build/dist/bin/static_file_server -d ../public -p 8080
./build/dist/bin/websocket_echo_server
./build/dist/bin/api_demo
```

## 端口说明

为避免冲突，不同示例使用不同的端口：

| 示例类型 | 默认端口 |
|---------|---------|
| 基础示例 | 8080 |
| 路由示例 | 8081 |
| 中间件示例 | 8082 |
| 静态文件示例 | 8083 |
| WebSocket 示例 | 8084 |
| 高级示例 | 8085 |

## 学习路径

建议按照以下顺序学习：

1. **01_basics** - 从最简单的 Hello World 开始
2. **02_routing** - 学习如何处理不同的 URL
3. **03_middleware** - 学习如何扩展服务器功能
4. **04_static_files** - 学习如何提供静态文件
5. **05_websocket** - 学习实时通信
6. **06_advanced** - 学习高级功能和最佳实践

## 相关文档

- [API 参考](../docs/api/)
- [开发者指南](../docs/guide/DEVELOPER_GUIDE.md)
- [教程](../docs/guide/TUTORIAL.md)
- [WebSocket 指南](../docs/guide/websocket.md)
- [静态文件服务](../docs/guide/STATIC_FILE_SERVER.md)
- [限流 API](../docs/guide/RATE_LIMIT_API.md)

## 注意事项

- 所有示例程序都遵循 UVHTTP 的设计原则
- 使用 `loop->data` 传递应用上下文（参考 [LIBUV_DATA_POINTER.md](../docs/guide/LIBUV_DATA_POINTER.md)）
- 使用统一的错误处理机制
- 遵循代码风格和命名约定

## 贡献

如果您有新的示例程序想法，欢迎提交 Pull Request！

## 许可证

MIT License