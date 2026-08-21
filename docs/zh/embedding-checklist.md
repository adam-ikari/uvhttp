# UVHTTP 嵌入验证清单

> 这份清单供嵌入者（将 uvhttp 作为 submodule 集成到自有项目中的开发者）使用。
> 每次重要发布或嵌入后，应逐项验证。

---

## 1. 构建集成

- [ ] 项目能以 submodule 方式成功克隆：`git clone --recurse-submodules`
- [ ] 项目能成功编译：`mkdir build && cd build && cmake .. && make`
- [ ] 嵌入者能通过 CMake `add_subdirectory` 或 `FetchContent` 集成
- [ ] 嵌入者能选择裁剪特性：`cmake .. -DBUILD_WITH_WEBSOCKET=OFF`
- [ ] 所有特性开关组合均能编译（见 [BUILD_CONFIGURATION_MATRIX.md](guide/BUILD_CONFIGURATION_MATRIX.md)）

## 2. 方法枚举映射

- [ ] 所有 HTTP 方法（GET/POST/PUT/DELETE/HEAD/OPTIONS/PATCH）在 router 中正确注册和匹配
- [ ] 使用 `uvhttp_any()` 注册的路由对所有请求方法生效
- [ ] 使用 `uvhttp_router_add_route()`（隐含 UVHTTP_ANY）注册的路由对所有请求方法生效
- [ ] 嵌入者的 HTTP 客户端库（如 undici、fetch）发送的请求方法被正确解析

## 3. 标准兼容性

- [ ] gzip 压缩输出的 `Content-Encoding: gzip` 能被标准 `gunzip` 解码
- [ ] gzip 流的 magic 字节为 `0x1f8b`（不是 zlib 的 `0x78`）
- [ ] WebSocket 握手头名大小写不敏感（`Sec-WebSocket-Key:` 与 `sec-websocket-key:` 均被接受）
- [ ] HTTP 头名大小写不敏感（`Content-Type:` 与 `content-type:` 均被接受）
- [ ] `Connection: Upgrade`、`connection: upgrade`、`CONNECTION: UPGRADE` 均被识别
- [ ] 控制帧（PING/PONG/CLOSE）payload 长度检查（RFC 6455 §5.5：≤125 字节）
- [ ] RSV 位校验（RFC 6455 §5.2：未协商扩展时 RSV 非零应关闭连接）

## 4. 静态文件服务

- [ ] 根路径 `/` 正确返回 index 文件（如 index.html）
- [ ] 子路径 `/static/file.txt` 正确返回文件
- [ ] 不存在的路径返回 404 且不崩溃
- [ ] 嵌入者配置的 index_file 无论是否有前导 `/` 均正常工作
- [ ] 大文件（>1MB）通过 sendfile 路径发送，无内存溢出

## 5. 处理管线

- [ ] 路由匹配时调用 handler
- [ ] 路由未匹配但 static 命中时调用 static handler
- [ ] 路由未匹配且 static 未命中时，若设置了 `server->handler`，回调该 handler（而非硬编码 404）
- [ ] 路由未匹配且 static 未命中且未设置 handler，返回 404
- [ ] 嵌入者 handler 可以正常发送响应（set_status、set_header、set_body、send）

## 6. 内存安全

- [ ] 嵌入者项目在 ASan 下运行无泄漏
- [ ] 嵌入者项目在 UBSan 下运行无未定义行为
- [ ] 长时间运行（>1 小时）无 RSS 增长
- [ ] 高并发（100 连接）下无 socket 错误
- [ ] 通过 `make verify-memory-safety` 验证

## 7. 多实例与隔离

- [ ] 同一进程启动多个 uvhttp_server_t 实例正常工作
- [ ] 每个实例的 gzip 缓存独立（无静态全局变量冲突）
- [ ] 每个实例的 router 独立
- [ ] 一个实例停止不影响其他实例

## 8. 特性组合验证

| 特性组合 | 验证 |
|---------|------|
| HTTP 纯文本 | [ ] |
| HTTP + WebSocket | [ ] |
| HTTP + 静态文件 | [ ] |
| HTTP + 压缩 | [ ] |
| HTTP + TLS | [ ] |
| HTTP + WebSocket + TLS | [ ] |
| HTTP + 路由缓存（ROUTER_CACHE=1） | [ ] |
| 所有特性全开 | [ ] |

## 9. 嵌入者接入清单（新嵌入者专用）

第一次将 uvhttp 作为 submodule 添加时，按以下步骤验证：

1. [ ] 在嵌入项目的 `CMakeLists.txt` 中添加 `add_subdirectory` 或 `FetchContent`
2. [ ] 编译嵌入项目，确认 uvhttp 头文件可引用
3. [ ] 创建最小 HTTP 服务器示例（~20 行），验证 `hello world` 响应
4. [ ] 运行 uvhttp 的测试套件：`cd <uvhttp>/build && ctest --output-on-failure`
5. [ ] 在嵌入项目上运行 ASan：`cmake -DENABLE_ASAN=ON .. && make && ./your-server`
6. [ ] 发送一个完整的 HTTP 请求并验证响应

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2026-08-20 | 初始版本，基于 qwrt 嵌入经验 |