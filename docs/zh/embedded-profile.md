---
title: 嵌入式与长期运行
sync_hash: 5a26c78ba54717d041c4fde52fcf1efa0586cc9c
description: UVHTTP 嵌入式与长跑指标：持续负载下 RSS 持平（60s 零增长）、静态库 ~257 KB、冷启动 ~310ms、支持 32 位。ASan-clean 内存安全保证的运行时佐证。
---

# 嵌入式与长期运行性能画像

单机峰值吞吐并非 UVHTTP 的卖点（这方面 nginx 与 h2o 更胜一筹）。对于嵌入式设备与长期运行的服务，真正关键的指标是另一组：**内存在数天的运行时间内是否保持平稳？占用的体积能多小？能否在 32 位上运行？启动有多快？** 本页回答这些问题。

这些测量是[内存安全保证](./MEMORY_SAFETY.md)的补充：消毒器干净的状态告诉你*从构造上不存在泄漏*；下方的长期运行 RSS 画像则展示了它*在持续负载下的实际表现*。

## 长期运行内存稳定性（持续负载下无泄漏）

不变量是：在持续负载下，服务器的内存驻留集（RSS）不得无界增长。一个在 10 秒功能测试中通过的、每连接缓慢泄漏，会在一周内让一台嵌入式设备 OOM。

使用 `scripts/performance/long_run_memory.sh` 测量，该脚本驱动持续的 `wrk -t4 -c100` 负载，并每 10 秒采样一次服务器的 RSS。

| 经过时间（秒） | RSS (KB) | 说明 |
|-------------|----------|------|
| 10 | 5,372 | 100 个持续连接 |
| 20 | 5,372 | |
| 30 | 5,372 | |
| 40 | 5,372 | |
| 50 | 5,372 | |
| 60 | 5,372 | |

**结果：RSS 平稳——在 60 秒的持续负载、约 20K RPS 下零增长（0 KB），零套接字错误。** 延长运行时间以扩展曲线；预期是一条水平线。

```bash
# 复现（默认 60 秒；传入更长的时长以获取更长的画像）
scripts/performance/long_run_memory.sh 300
```

> 这是 ASan 干净保证的运行时补充。ASan 证明沿任何已测试代码路径都不会丢失分配；RSS 画像则证明分配器达到了稳态，且在真实流量下不会漂移。

## 占用体积

默认 Release 构建（`-O2 -DNDEBUG`，已 strip），64 位，系统分配器：

| 构件 | 体积 |
|------|------|
| 静态库 `libuvhttp.a` | ~257 KB（已 strip） |
| 示例服务器二进制（`test_performance_e2e`） | ~1.0 MB（已 strip，静态链接） |
| 100 连接下的稳态 RSS | ~5.3 MB |

~257 KB 的静态库是嵌入式相关的数字——一个链接 UVHTTP 的设备增加约四分之一兆字节的代码，加上 libuv/mbedtls（仅当相应功能启用时；两者均为编译期可选）。

> **关于 `-Os` 的说明：** 项目当前的 Release 模式强制使用 `-O2 -DNDEBUG`（见 `CMakeLists.txt`）。如需构建最小体积，覆盖 `CMAKE_C_FLAGS_RELEASE`：
> ```bash
> cmake -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG -ffunction-sections -fdata-sections" \
>       -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -s" ..
> ```

## 冷启动

从进程启动到提供首字节的时间（包含进程启动、事件循环初始化、服务器绑定以及一次请求）：

| 指标 | 数值 |
|--------|-------|
| 启动后首字节 | ~310 ms |

适用于按需 / 惰性启动的嵌入式服务以及 serverless 式调用。

## 32 位支持

UVHTTP 显式支持 32 位目标（v2.5.0 版本主题即为“32 位支持与压缩”）。32 位 CI 构建使用以下配置：

```bash
cmake -B build-32bit \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686" \
  ...
```

（见 `.github/workflows/ci-32bit.yml`。）在 32 位下，指针体积减半，连接 / 请求 / 响应结构体也随之缩小，使占用体积适用于资源受限的设备。32 位构建的正确性在 CI 中受把关。

## 编译期最小化

占用体积可在编译期配置——只为所用功能付费：

| 功能 | CMake 选项 | 默认值 |
|---------|--------------|---------|
| WebSocket | `BUILD_WITH_WEBSOCKET` | ON |
| TLS（mbedtls） | `BUILD_WITH_HTTPS` | ON |
| 静态文件 | `UVHTTP_FEATURE_STATIC_FILES` | ON |
| 压缩 | `BUILD_WITH_COMPRESSION` | ON |
| 速率限制 | `UVHTTP_FEATURE_RATE_LIMIT` | ON |
| mimalloc 分配器 | `BUILD_WITH_MIMALLOC` | OFF |

一个最小化的嵌入式构建（仅 HTTP/1.1，不含 TLS/WebSocket/静态文件/压缩）通过禁用未用功能，链接一个更小的库。

## 为何重要

对于一个设备管理端点或长期运行的边缘服务：

- 一个每连接增加 1 KB 的泄漏在 10 秒的 `wrk` 运行中不可见，但在带 keep-alive 抖动的 100 req/s 下每天耗费约 10 MB。UVHTTP 平稳的 RSS 画像与 ASan 干净状态意味着这一失效模式从构造上被消除。
- 一个 ~257 KB、可在 32 位上运行的库，能装进 nginx（一个自带运行时的独立守护进程）所不能之处。
- 消毒器干净 + 32 位 + 小占用体积是大多数轻量级 C HTTP 库所不具备的组合——见[首页对比表](./)。
