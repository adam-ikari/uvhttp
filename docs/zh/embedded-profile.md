---
title: 嵌入式与长期运行
description: UVHTTP 嵌入式与长跑指标：持续负载下 RSS 持平（60s 零增长）、静态库 ~257 KB、冷启动 ~310ms、支持 32 位。ASan-clean 内存安全保证的运行时佐证。
---

# 嵌入式与长期运行性能画像

UVHTTP 不比单机峰值吞吐，nginx 和 h2o 在那一项上更强。设备要连续跑几个月，或者服务长期在线，决定成败的是另一组数字：RSS 会不会一直涨、二进制多大、32 位能不能跑、启动要多久。这一页逐个量给你看。

这些测量补上[内存安全保证](./MEMORY_SAFETY.md)缺的另一半：消毒器干净，说明已测试的路径上不丢内存；下面的 RSS 曲线，说明真实流量下内存也不会漂。

## 长期运行内存稳定性（持续负载下无泄漏）

要求很直接：持续负载下，服务器的常驻内存（RSS）不能无界增长。一个每连接慢漏几 KB 的问题，能通过 10 秒的功能测试，却会在一周内把嵌入式设备拖到 OOM。

测量用 `scripts/performance/long_run_memory.sh`，它持续发 `wrk -t4 -c100` 负载，每 10 秒采样一次服务器 RSS。

| 经过时间（秒） | RSS (KB) | 说明 |
|-------------|----------|------|
| 10 | 5,372 | 100 个持续连接 |
| 20 | 5,372 | |
| 30 | 5,372 | |
| 40 | 5,372 | |
| 50 | 5,372 | |
| 60 | 5,372 | |

**结果：RSS 持平。60 秒持续负载、约 20K RPS，增长 0 KB，没有套接字错误。** 想看得更久就把时长拉长，曲线应该一直是一条水平线。

```bash
# 复现（默认 60 秒；传入更长的时长以获取更长的画像）
scripts/performance/long_run_memory.sh 300
```

> ASan 证明已测试路径上不丢分配；RSS 画像证明分配器能在真实流量下进入稳态并停在那里。

## 占用体积

默认 Release 构建（`-O2 -DNDEBUG`，已 strip），64 位，系统分配器：

| 构件 | 体积 |
|------|------|
| 静态库 `libuvhttp.a` | ~257 KB（已 strip） |
| 示例服务器二进制（`test_performance_e2e`） | ~1.0 MB（已 strip，静态链接） |
| 100 连接下的稳态 RSS | ~5.3 MB |

设备上真正要看的是 ~257 KB 这个静态库数字：链接 UVHTTP 大约多出四分之一兆字节代码，libuv 和 mbedtls 只在启用对应功能时才计入。

> **关于 `-Os`：** 项目当前的 Release 模式固定使用 `-O2 -DNDEBUG`（见 `CMakeLists.txt`）。要更小的体积，覆盖 `CMAKE_C_FLAGS_RELEASE`：
> ```bash
> cmake -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG -ffunction-sections -fdata-sections" \
>       -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -s" ..
> ```

## 冷启动

从进程启动到返回首字节，包含进程启动、事件循环初始化、服务器绑定和一次请求：

| 指标 | 数值 |
|--------|-------|
| 启动后首字节 | ~310 ms |

按需启动、惰性启动的嵌入式服务，以及 serverless 式调用，都能接受这个时间。

## 32 位支持

UVHTTP 支持 32 位目标，v2.5.0 的发布主题就是“32 位支持与压缩”。32 位构建这样配置：

```bash
cmake -B build-32bit \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686" \
  ...
```

32 位下指针减半，连接、请求、响应结构体随之缩小，内存紧张的设备能放下。`.github/workflows/ci-32bit.yml` 并不在当前的 CI 工作流里，32 位构建需要自己跑。

## 编译期最小化

体积是编译期的选择，用多少链接多少：

| 功能 | CMake 选项 | 默认值 |
|---------|--------------|---------|
| WebSocket | `BUILD_WITH_WEBSOCKET` | ON |
| TLS（mbedtls） | `BUILD_WITH_HTTPS` | ON |
| 静态文件 | `BUILD_WITH_STATIC_FILES` | OFF |
| 压缩 | `BUILD_WITH_COMPRESSION` | ON |
| mimalloc 分配器 | `BUILD_WITH_MIMALLOC` | OFF |

只做 HTTP/1.1 的最小嵌入式构建关掉 TLS、WebSocket、静态文件和压缩，链接的库会小一截。

## 为何重要

设备管理端点、长期在线的边缘服务，都会碰到这几个问题：

- 每连接漏 1 KB，10 秒的 `wrk` 跑不出来；100 req/s 加上 keep-alive 抖动，一天就是约 10 MB。UVHTTP 的 RSS 曲线和 ASan 干净状态把这个失效模式从根上拿掉了。
- 一个 ~257 KB、能跑 32 位的库，能塞进 nginx 塞不进去的地方；nginx 是自带运行时的独立守护进程。
- 消毒器干净、32 位、小体积，这三样多数轻量 C HTTP 库最多占两样。首页的[对比表](./)列了细节。
