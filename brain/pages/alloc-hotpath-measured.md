---
id: alloc-hotpath-measured
title: "每请求分配已实测：3 次，省一次仅 0.026% —— 内存分配优化不做"
category: decision
status: active
tags: [performance, benchmark, allocator, methodology, yagni]
created: "2026-09-30T11:02:57"
updated: "2026-09-30T11:03:30"
---

<!-- compiled_truth -->
# 每请求分配已实测：省一次仅 0.026%，内存分配优化不做

## 结论

v2.9.x 的 P2「内存分配优化 / 减少热路径分配次数」**应当关闭（不做实现）**。不是"暂缓"，是**实测收益低于测量噪声**，做它只增加风险不产生可观测收益。

## 实测数据

用 `LD_PRELOAD` 包装 malloc/free + `SIGUSR1` dump 计数，`benchmark_unified` 本机压测：

| 场景 | 每请求 malloc |
|---|---|
| curl（每请求新连接） | **9** |
| keep-alive 复用连接（`/` 小 body，copy 路径） | **3** |
| keep-alive 2000 请求单连接，结束 live 计数 | **3**（连接本身），**无泄漏** |

3 次分配的来源（小 body copy 路径）：
1. `uvhttp_response_set_body` → `response->body`
2. `uvhttp_response_prepare` → `headers_buffer`（1024B scratch，填完 memcpy 进 `response_data` 后立即 free）
3. `uvhttp_response_build_data` → `response_data`（header+body 拼接，即 write_data 缓冲）

新连接的 9 次 = 连接建立 7 次（connection_t / read_buffer / request_t / response_t / parser_settings / parser / body）+ 响应 2 次。

## 收益上限

唯一可安全消除的是第 2 项（1024B scratch，可在 response 上复用）。但：

- 实测 `malloc(1024)+free` = **~12ns**（`-O2`，volatile 防消除，三次 12.05/11.87/11.44ns）
- 单连接 keep-alive 每请求 **45.7µs**（21885 RPS @ `-t1 -c1`）
- 12ns / 45.7µs = **0.026%**

这个量级低于本项目 runner 跨 run 方差（~40%，见 `benchmark-thermal-throttling`）三个数量级，**任何门禁都测不出来**。

## 为什么不值得做

scratch buffer 复用不是纯粹的省一次 malloc：零拷贝路径下 `headers_buffer` 是 `uv_write` 的第一个 iovec，**必须活到 write 回调**才能释放。要复用就得在 response 上加"in-use 直到 write 完成"的状态，并保证写完成前不进入下一轮。单线程事件循环下这可行，但引入了一个新的生命周期约束——而收益是 0.026%。

拿一个跨异步写完成的生命周期约束去换测不出来的 0.026%，是负期望。

## 可迁移的判据

**给性能优化立项前先算上限。** 步骤：
1. 数出热路径每请求的真实分配次数（LD_PRELOAD + 信号 dump，别靠读代码猜——`curl` 新连接 9 次和 keep-alive 3 次差 3 倍，只读代码会数错）
2. 实测单次 alloc+free 的 ns（本机 glibc 约 12ns / 1024B）
3. 上限 = 省下的次数 × 单次开销 ÷ 单请求总耗时
4. 上限 < 测量方差的 1/10 → 不做

第 4 步是硬门槛。本项目绝对 RPS 门禁已被证明 gate 的是 runner 运气（见 `perf-regression-gate`），跨 run 方差 ~40%。

## 顺带确认（无缺陷）

同一轮顺路核了几个安全面，均**无缺口**，避免以后重复排查：
- `headers_extra` 扩容路径：走同一条 `set_header`，无独立入口，校验完整
- `uvhttp_safe_strcpy`：snprintf 保证 NUL 终止，截断安全
- `set_header` 的 `MAX_HEADERS` 上限：`new_capacity` 被钳制，达到上限返回 OOM，无整数溢出
- 静态文件穿越：`realpath` 规范化 + `strncmp` 前缀比对 + `/` 边界检查，fopen 用的是 canonical 路径
- `on_url` / `on_header_value` 续段用 `strlen` 判断累积长度（与 `on_header_field` 的显式计数不一致）：**llhttp 在解析层就拒绝含 NUL 的 URL（rc=7 HPE_INVALID_URL）和 NUL header value（rc=10）**，回调拿不到 NUL，缺陷不可达。若将来启用 llhttp lenient 模式，这条会变成真实问题——改 lenient 时必须回头处理


## Timeline

- time: 2026-09-30T11:02:57
  kind: decision
  summary: "Created this page: 每请求分配已实测：3 次，省一次仅 0.026% —— 内存分配优化不做"
  source: "内存分配优化 P2 调研（2026-09-30）"
  affects: [alloc-hotpath-measured]

- time: 2026-09-30T11:03:30
  kind: decision
  summary: "每请求分配实测：keep-alive 3 次/请求、无泄漏；唯一可省的 1024B scratch 上限 0.026%，远低于 runner 方差 40%，故内存分配优化不做"
  source: "内存分配优化 P2 调研（2026-09-30）"
  affects: [alloc-hotpath-measured]
