---
id: io-uring-evaluation
title: "io_uring 评估：当前架构不可达，待办关闭"
category: decision
status: active
tags: [performance, architecture, io-uring, libuv, yagni]
created: "2026-09-30T14:22:03"
updated: "2026-09-30T14:25:14"
---

<!-- compiled_truth -->
# io_uring 评估：当前架构下不可达，待办关闭

## 结论

v2.9.x 的 P2「io_uring 探索 / 评估 io_uring 替代 epoll 在静态文件路径中的收益」**关闭**。不是收益低于噪声（如 `alloc-hotpath-measured`），而是**路径上不可达**——在 libuv 架构内 io_uring 覆盖不到 uvhttp 的静态文件热路径，要真正受益需绕过 libuv 自管 fd，属于架构重构而非优化。

## libuv 1.52 io_uring 覆盖现状

libuv 1.52（本项目依赖版本）已内置 io_uring 代码（`deps/libuv/src/unix/linux.c`），覆盖 `uv_fs_*` 操作的 open/read/write/close/statx/fsync（`fs.c:1820-2046`）。启用方式：`UV_USE_IO_URING=1` 环境变量（默认关闭；SQPOLL 还需 `UV_LOOP_USE_IO_URING_SQPOLL` loop flag，且要求 kernel ≥ 5.10.186，因更老内核 SQPOLL 线程会 100% CPU）。

**唯独不覆盖 sendfile**：`uv__fs_sendfile`（`fs.c:1029-1053`）是同步 `sendfile(2)` syscall，在 libuv 线程池执行——不走 io_uring。libuv 至 1.52 未实现 sendfile 的 io_uring 路径。

## uvhttp 静态文件路径与 io_uring 的关系

uvhttp 的静态文件大文件路径（`src/uvhttp_static.c`）用 `uv_fs_sendfile` 分块传输，每块 256KB（`UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE`），每块一次 `uv_fs_sendfile` 提交 = 一次线程池往返 + 一次 `sendfile(2)` syscall。

io_uring 对这两个开销都帮不上：

1. **sendfile syscall 本身**——io_uring 要替代需用 `IORING_OP_SPLICE` 或 `IORING_OP_SEND_ZC`，但 libuv 不提供这层，得绕过 libuv 自管 ring fd / completion / fd 生命周期，与 event loop 集成。这是架构级重构。
2. **线程池切换**——`uv_fs_sendfile` 在 libuv 设计上强制走线程池（与 read/write 不同），即使内核支持 io_uring sendfile，libuv 也不会用它。要享受 io_uring 得绕过 libuv 的 fs 提交路径。
3. **小文件路径**——uvhttp 用手写 `open/read/close` 同步 syscall（`uvhttp_static.c:1760+`，走 `uvhttp_static_sendfile_with_config` 的小文件分支），不经 libuv fs API，io_uring 天然不生效。

## 基准门禁覆盖不到

CI 配对门禁的 3 个端点（`/`、`/json`、`/large`）全是内存 body——`/large` 是 100KB 内存缓冲（`benchmark/benchmark_unified.c:284`），不走 sendfile。sendfile 只在 nightly `performance-full` 的 `file/large.bin` 端点跑，且那是绝对值（不参与配对 gate，brain 已记录绝对值跨 run 不可比）。

**即 io_uring 即便引入，也无法在门禁上被验证**——门禁测的是内存响应路径，sendfile 路径不进 gate。

## 两条路都不满足 YAGNI

| 路径 | 做法 | 代价 | 评价 |
|---|---|---|---|
| 改 sendfile 为 `uv_fs_read` | 享受 io_uring 的 read 路径 | 放弃 sendfile 零拷贝（内核态 splice），自废武功 | 负收益 |
| 原生 io_uring splice | 绕过 libuv 自管 ring fd | 重写 fd 生命周期、与 event loop 集成、自己处理 completion | 架构重构，上限未量化，且无门禁能验证 |

## 关闭判据

与 `alloc-hotpath-measured` 不同——那个是「收益低于噪声」可量化拒绝；这个是「路径不可达」：在不重构 libuv 集成的前提下，io_uring 触不到 uvhttp 的静态文件热路径。需要重构才能评估上限，重构本身有明确代价而无明确收益预期，YAGNI 关闭。

## 何时重启评估

任一条件成立才重开：
- libuv 上游实现 sendfile 的 io_uring 路径（届时 `uv_fs_sendfile` 透明受益，零改动）
- 项目脱离 libuv 自管 event loop（不再适用本评估）
- 出现静态文件基准进入配对门禁的需求（届时先量化 sendfile 在目标负载下的开销占比）


## Timeline

- time: 2026-09-30T14:22:03
  kind: decision
  summary: "Created this page: io_uring 评估：当前架构不可达，待办关闭"
  source: "io_uring 探索 P2 评估（2026-09-30）"
  affects: [io-uring-evaluation]

- time: 2026-09-30T14:25:14
  kind: decision
  summary: "io_uring 在当前 libuv 架构下不可达：libuv 1.52 不覆盖 sendfile，uvhttp 静态文件热路径全走 sendfile+线程池；要受益需绕过 libuv 自管 ring fd，属架构重构。关闭待办"
  source: "io_uring 探索 P2 评估（2026-09-30）"
  affects: [io-uring-evaluation]
