# UVHTTP 嵌入示例（新嵌入者接入）

这是一个**独立可运行**的最小嵌入项目，演示如何在自己的 CMake 项目中集成
uvhttp（静态库）。它不参与 uvhttp 主构建（`BUILD_EXAMPLES` 不会编译本目录），
而是作为一个独立的 CMake 工程，供你直接复制参考。

## 目录结构

```
embedding/
├── CMakeLists.txt   # add_subdirectory 方式嵌入 uvhttp
├── main.c           # 最小 HTTP 服务器（路由 / → hello world，优雅退出）
└── README.md        # 本文件
```

## 前置条件

uvhttp 的 `deps/` 均为 git submodule，确保子模块已就绪：

```bash
git submodule update --init --recursive
```

## 构建与运行

本示例通过相对路径 `../..` 引用仓库内的 uvhttp 源码，因此可独立配置构建：

```bash
cmake -S examples/embedding -B /tmp/embed-build
cmake --build /tmp/embed-build -j
/tmp/embed-build/embedding_server          # 默认 0.0.0.0:8080
# 端口可通过命令行参数指定：
/tmp/embed-build/embedding_server 8090     # 0.0.0.0:8090
```

在另一个终端验证：

```bash
curl http://localhost:8080/
# Hello from embedded uvhttp!
```

按 `Ctrl+C`（或 `kill -TERM`）优雅退出：`uv_signal` 处理 SIGINT/SIGTERM，
`uvhttp_server_free` 完成清理后进程退出。

> 注意：本示例的 `main.c` 在 `uv_run` 返回后、调用 `uvhttp_server_free` 之前，
> 先关闭自建的 `uv_signal` 句柄。这是 uvhttp 嵌入的一个要点：
> `uvhttp_server_free` 内部会跑 `UV_RUN_ONCE` 清理循环，若 loop 中仍残留其他
> 活跃句柄会导致其阻塞。另外，`uvhttp_server_free` 会一并释放其持有的
> router，因此不要重复调用 `uvhttp_router_free`。

## 在你的项目里复制

1. 把本目录的 `main.c` 与 `CMakeLists.txt` 拷到你的项目。
2. 把 uvhttp 放进你的目录树，例如 `third_party/uvhttp`：
   ```bash
   git submodule add https://github.com/adam-ikari/uvhttp.git third_party/uvhttp
   git -C third_party/uvhttp submodule update --init --recursive
   ```
3. 修改 `CMakeLists.txt` 中的 `UVHTTP_SOURCE_DIR` 指向你的 uvhttp 目录。
4. 按需裁剪特性：`cmake -DBUILD_WITH_WEBSOCKET=OFF -DBUILD_WITH_HTTPS=OFF ..`

## 完整接入指南

- [英文：docs/guide/EMBEDDING_GUIDE.md](../../docs/guide/EMBEDDING_GUIDE.md)
- [中文：docs/zh/guide/EMBEDDING_GUIDE.md](../../docs/zh/guide/EMBEDDING_GUIDE.md)
- [嵌入验证清单](../../docs/embedding-checklist.md)
- [嵌入式配置](../../docs/embedded-profile.md)
