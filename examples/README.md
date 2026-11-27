# UVHTTP 示例程序

`examples/` 下有 52 个 `.c` 示例，由一个 Makefile 自动发现并构建。目标名就是文件 basename（52 个全部唯一），不存在需要单独登记的目标。

## 先构建库

示例链接的 feature 宏必须和库的构建一致，否则 ABI 不匹配。默认 `build/` 关掉了 static / LRU / router cache，而示例按功能超集写，统一用 `build_static/`：

```bash
# 仓库根目录
cmake -S . -B build_static -DBUILD_WITH_STATIC_FILES=ON -DBUILD_WITH_ROUTER_CACHE=ON
cmake --build build_static -j$(nproc)
```

## 编译示例

从 `examples/` 目录执行：

```bash
cd examples
make -f Makefile.examples           # 全部 52 个
make -f Makefile.examples help      # 列出所有目标
make -f Makefile.examples api_demo  # 单个示例
```

产物在 `examples/bin/`。库是静态 `.a`，运行时不需要 `LD_LIBRARY_PATH`。

调试构建：`make -f Makefile.examples BUILD_TYPE=debug`。

## 目录结构

```
examples/
├── 01_basics/           6 个  基础 HTTP 服务器
├── 02_routing/          2 个  路由与方法分发
├── 03_middleware/       5 个  中间件链、限流
├── 04_static_files/     5 个  静态文件与缓存
├── 05_websocket/        3 个  WebSocket 回显与连接管理
├── 06_advanced/        10 个  配置、JSON、升级、统一响应、分配器
├── 07_performance/      4 个  压缩、HTTPS、WebSocket 性能
├── 08_e2e_tests/       13 个  各功能的端到端测试
├── 10_embedding/        1 个  生产 embedder 示例 + Web 管理台
├── embedding/           1 个  早期嵌入示例
├── simple_server.c            单体服务器示例
├── custom_compression_strategy.c
└── Makefile.examples
```

## 生产示例：production_app

`production_app` 展示 embedder 实际会用到的接缝：config 单通道（`take_config` / `get_config`）、请求过滤、错误回调、`get_stats`、`set_draining` + `async stop`、原子响应（`uvhttp_respond`）、由应用层实现的日志接口、静态文件 catch-all。端口必填，不设默认值：

```bash
cd examples
make -f Makefile.examples production_app
./bin/production_app 18080
```

Web 管理台文件在 `10_embedding/www/`（`index.html`、`dashboard.html`）。

```bash
curl localhost:18080/
curl localhost:18080/json
curl localhost:18080/metrics
curl localhost:18080/index.html
curl -H 'X-Deny: 1' localhost:18080/        # 403，请求过滤生效
curl -X POST localhost:18080/admin/drain    # 进入 draining
kill -TERM <pid>                            # drain 后停止
```

## 各目录主要示例

| 目录 | make 目标 | 说明 |
|------|-----------|------|
| 01_basics | `helloworld` / `01_hello_world` / `helloworld_complete` | 从最简到带完整错误处理 |
| 02_routing | `01_simple_routing` / `02_method_routing` | 路径与方法路由 |
| 03_middleware | `middleware_chain_demo` / `rate_limit_demo` | 中间件链与限流 |
| 04_static_files | `static_file_server` / `cache_test_server` | 静态服务与缓存 |
| 05_websocket | `websocket_echo_server` | WebSocket 回显 |
| 06_advanced | `api_demo` / `json_api_demo` / `unified_response_demo` | 高级 API 用法 |
| 07_performance | `compression_server` / `performance_https_server` | 压缩与 TLS 性能 |
| 08_e2e_tests | `test_simple_e2e` / `test_websocket_simple_e2e` | 端到端测试程序 |
| 10_embedding | `production_app` | 生产 embedder + Web 管理台 |

完整目标列表见 `make -f Makefile.examples help`。

## 端口

教学示例多数把端口硬编码为 `8080`，仅用于本机演示，同时运行多个会冲突。`production_app` 的端口由命令行参数必填（`1..65535`），并行运行不会撞端口。早期示例（如 `02_method_routing`）可能用其他固定端口，运行前看源码开头的 curl 示例。

## 学习路径

1. `01_basics` — 最简服务器与完整错误处理
2. `02_routing` — URL 与 HTTP 方法分发
3. `03_middleware` — 中间件扩展
4. `04_static_files` — 静态文件与缓存
5. `05_websocket` — 实时通信
6. `06_advanced` — 配置、JSON、协议升级、统一响应
7. `10_embedding/production_app` — 把上面这些接缝拼成可运行的应用

## 相关文档

- [API 参考](../docs/api/API_REFERENCE.md)
- [开发者指南](../docs/guide/DEVELOPER_GUIDE.md)
- [嵌入指南](../docs/guide/EMBEDDING_GUIDE.md)
- [教程](../docs/guide/TUTORIAL.md)
- [WebSocket 指南](../docs/guide/websocket.md)
- [静态文件服务](../docs/guide/STATIC_FILE_SERVER.md)
- [请求过滤器 API](../docs/guide/REQUEST_FILTER_API.md)

## 注意事项

- 应用上下文通过 `loop->data` 传递，参考 [LIBUV_DATA_POINTER.md](../docs/guide/LIBUV_DATA_POINTER.md)
- 日志接口只声明、不实现；embedder 自行实现 `uvhttp_log_set_sink`，或定义 `UVHTTP_LOG_EMIT` 重定向到自己的 logger
- 响应体字面量用 `strlen` 给出长度，中文按 UTF-8 字节计，不手数字符数
- 新增示例时只要放一个 `.c` 文件，Makefile 会自动发现，无需登记目标

## 贡献与许可证

欢迎提交新的示例程序。许可证 MIT，见仓库 LICENSE 文件。
