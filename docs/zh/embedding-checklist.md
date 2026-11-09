# UVHTTP 嵌入验证清单

> 这份清单供嵌入者使用：把 uvhttp 作为 submodule 集成到自有项目的开发者。
> 每次重要发布或每次嵌入后，逐项验证。

---

## 1. 构建集成

- [x] 项目能以 submodule 方式克隆：`git clone --recurse-submodules`
- [x] 项目能编译：`mkdir build && cd build && cmake .. && make`
- [x] 嵌入者能通过 CMake `add_subdirectory` 集成
- [x] 嵌入者能裁剪特性：`cmake .. -DBUILD_WITH_WEBSOCKET=OFF`
- [ ] 所有特性开关组合均能编译（见 [BUILD_CONFIGURATION_MATRIX.md](guide/BUILD_CONFIGURATION_MATRIX.md)）

## 2. 方法枚举映射

- [ ] 所有 HTTP 方法（GET/POST/PUT/DELETE/HEAD/OPTIONS/PATCH）在 router 中正确注册和匹配
- [ ] 在 `uvhttp_server_listen_routes()` 路由表中以 `UVHTTP_ANY` 方法注册的路由匹配所有请求方法
- [ ] 以 `uvhttp_router_add_route()`（隐含 UVHTTP_ANY）注册的路由匹配所有请求方法
- [ ] 嵌入者的 HTTP 客户端库（如 undici、fetch）发送的请求方法解析正确

## 3. 标准兼容性

- [ ] gzip 输出带 `Content-Encoding: gzip`，标准 `gunzip` 能解码
- [ ] gzip 流的 magic 字节为 `0x1f8b`（不是 zlib 的 `0x78`）
- [ ] WebSocket 握手头名大小写不敏感（`Sec-WebSocket-Key:` 与 `sec-websocket-key:` 均被接受）
- [ ] HTTP 头名大小写不敏感（`Content-Type:` 与 `content-type:` 均被接受）
- [ ] `Connection: Upgrade`、`connection: upgrade`、`CONNECTION: UPGRADE` 均被识别
- [ ] 控制帧（PING/PONG/CLOSE）payload 长度检查（RFC 6455 §5.5：≤125 字节）
- [ ] RSV 位校验（RFC 6455 §5.2：未协商扩展时 RSV 非零则关闭连接）

## 4. 静态文件服务

- [ ] 根路径 `/` 返回 index 文件（如 index.html）
- [ ] 子路径 `/static/file.txt` 返回该文件
- [ ] 不存在的路径返回 404 且不崩溃
- [ ] 嵌入者配置的 index_file 无论有无前导 `/` 均正常工作
- [ ] 大文件（>1MB）走 sendfile 路径，无内存膨胀

## 5. 处理管线

- [ ] 路由匹配时调用对应 handler
- [ ] 路由未匹配但 static 命中，由 static handler 处理
- [ ] 路由未匹配且 static 未命中：若设置了 `server->handler`，调用该 handler，而非硬编码 404
- [ ] 路由未匹配、static 未命中且未设置 handler：返回 404
- [ ] 嵌入者 handler 能正常发送响应（set_status、set_header、set_body、send）

## 6. 内存安全

- [ ] 嵌入者项目在 ASan 下运行无泄漏
- [ ] 嵌入者项目在 UBSan 下运行无未定义行为
- [ ] 长时间运行（>1 小时）RSS 无增长
- [ ] 高并发（100 连接）下无 socket 错误
- [ ] `make verify-memory-safety` 通过

## 7. 多实例与隔离

- [ ] 同一进程内多个 uvhttp_server_t 实例正常工作
- [ ] 每个实例的 gzip 缓存独立（无静态全局变量冲突）
- [ ] 每个实例的 router 独立
- [ ] 停止一个实例不影响其他实例

## 8. 特性组合验证

| 特性组合 | 已验证 |
|---------|------|
| 纯 HTTP | [ ] |
| HTTP + WebSocket | [ ] |
| HTTP + 静态文件 | [ ] |
| HTTP + 压缩 | [ ] |
| HTTP + TLS | [ ] |
| HTTP + WebSocket + TLS | [ ] |
| HTTP + 路由缓存（ROUTER_CACHE=1） | [ ] |
| 所有特性全开 | [ ] |

## 9. 嵌入者接入清单（新嵌入者）

第一次把 uvhttp 作为 submodule 加入时，验证以下步骤。
完整接入指南见 [EMBEDDING_GUIDE.md](guide/EMBEDDING_GUIDE.md)，
可运行的独立示例见 [examples/embedding/](https://github.com/adam-ikari/uvhttp/tree/main/examples/embedding)。

1. [ ] 在嵌入项目的 `CMakeLists.txt` 中添加 `add_subdirectory` 或 `FetchContent`
2. [ ] 编译嵌入项目，确认 uvhttp 头文件可引用
3. [ ] 创建最小 HTTP 服务器（约 20 行，可直接参考 `examples/embedding/main.c`），确认返回 `hello world`
4. [ ] 运行 uvhttp 的测试套件：`cd <uvhttp>/build && ctest --output-on-failure`
5. [ ] 在嵌入项目上运行 ASan：`cmake -DENABLE_ASAN=ON .. && make && ./your-server`
6. [ ] 发送完整的 HTTP 请求并验证响应

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2026-08-20 | 初始版本，基于在 qwrt 中嵌入 uvhttp 的经验 |
| 1.1 | 2026-08-25 | 第二轮验证：add_subdirectory 集成，CMake 依赖可见性修复 |
| 1.2 | 2026-08-27 | 新增嵌入者接入内容：独立 `examples/embedding/` 示例与完整的 `EMBEDDING_GUIDE.md` |
