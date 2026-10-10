---
id: api-atomization-orthogonalization
title: "v2.10 方向：API 原子化与正交化，停止新增特性"
category: decision
status: active
tags: [api, design, roadmap, v2.10]
created: "2026-10-09T00:27:12"
updated: "2026-10-09T00:46:40"
---

<!-- compiled_truth -->
# v2.10 方向：API 原子化与正交化

## 决定

1. **停止新增特性。** v2.10.x 唯一方向是把现有公共 API 做**原子化**与
   **正交化**改造 —— 不加能力，把 2.x 攒下的耦合在 3.0 之前还清。

2. **兼容策略 = B：2.10 直接破坏性改**（2026-10-09 用户拍板）。
   **不留 deprecated 兼容层，不做新旧 API 并行。**

3. **第一性原理结论：v2.10 的主攻方向不是"合并配置通道"，而是
   "让配置成为真的控制面，或者删掉它假装的那部分"**（2026-10-09 核实后修正）。
   原 roadmap 把"配置双通道合并"列为第一刀是基于错误诊断 —— 见下。

## 为什么是现在

v2.9.x 的实测结论把"加功能"这条路堵死了：性能（[[alloc-hotpath-measured]]
0.026%、[[io-uring-evaluation]] 架构不可达）、测试（[[silent-skip-audit-101-to-18]]
101→18 已完成）、CI（[[sanitizer-feature-blindspot]] 盲区已修）三条抓手各自见底。
剩下唯一有价值的抓手是 API 自身的形状 —— 但见下，**它比预想的大得多**。

## 核心发现：config 是装饰品，不是控制面

### 诊断的修正过程

初版诊断是"两条配置通道（`server->config` vs `context->current_config`）
可能互相矛盾"。**这个诊断是错的**，因为它假设 config 大体上是活的、
只是有两个来源。

逐字段核实（脚本按 `config->field` 限定匹配，排除 `conn->field` 同名字段干扰）
后得到的真相：**`uvhttp_config_t` 26 个字段中，14 个从不被任何执行路径读取。**

| 分类 | 数量 | 字段 |
|---|---|---|
| 真读（config→执行点） | 11 | `max_connections` `backlog` `connection_timeout` `max_file_size` `trust_proxy_headers` `websocket_max_frame_size/message_size/ping_interval/ping_timeout` `tcp_keepalive_timeout` `sendfile_timeout_ms` `sendfile_max_retry` |
| **仅写不读**（API 写进去，无人读） | 3 | `max_body_size` `request_timeout` `keepalive_timeout` |
| **零读点**（外部代码从不访问） | 11 | `read_buffer_size` `max_header_size` `max_url_size` `max_requests_per_connection` `rate_limit_window` `rate_limit_max_requests` `rate_limit_max_window_seconds` `rate_limit_min_timeout_seconds` `cache_default_max_entries` `cache_default_ttl` `lru_cache_batch_eviction_size` |

### 为什么这是设计缺陷而非疏漏

`uvhttp_config_set_defaults` 给全部 26 个字段赋默认值，
`uvhttp_config_validate` 校验全部 26 个的范围，`uvhttp_config_print` 打印全部
26 个。**没有任何一个环节承认其中 14 个不影响行为。**

于是形成这个模式：

- `uvhttp_set_max_body_size(builder, n)` → 写 `config->max_body_size` → 返回 builder
- `uvhttp_config_update_size_limits(ctx, n, m)` → 写同一字段 → **打 INFO 日志
  "Limits updated - Body: %zu -> %zu"** → 返回 `UVHTTP_OK`
- 实际执行：`request.c:308` 用编译期常量 `UVHTTP_MAX_BODY_SIZE` 判断 body 超限

**两个公共 API 报告成功、打日志、返回 OK，而对服务器行为零影响。**
这不是"可能失效"，是**必然失效且静默** —— 比初版诊断严重一个量级。

`uvhttp_set_timeout` 同样：`request_timeout` / `keepalive_timeout` 仅写不读，
实际超时由 `connection.c:1553` 读 `config->connection_timeout`（**第三个字段**）。
所以 builder 的 `uvhttp_set_timeout` 设的数和实际生效的数**不是同一个字段**。

限流整块也是：`config` 里 4 个 `rate_limit_*` 字段零读点，
`uvhttp_server_enable_rate_limit` 把值写进 `server->rate_limit_*` 字段。
配置对象和真实状态是两套，且配置那套完全没人看。

### 第一性原理：这是"配置"还是"文档"？

`uvhttp_config_t` 目前的行为更接近**一份服务器规格说明书**，而非控制面：
它的字段被默认值填充、被校验、被打印、被写，然后被忽略。

由此得出 v2.10 的真问题，不是"怎么合并两个通道"，而是二选一：

- **让它成真**：14 个字段要么接到执行点，要么删掉。接到执行点意味着
  `read_buffer_size` / `max_body_size` 要进 per-connection 分配与热路径 ——
  **这是新增运行时可调能力，与"特性冻结"矛盾，且触及性能敏感区**。
- **承认它只是规格**：删掉假的（14 个字段 + 3 个假 API），只留真生效的 11 个。
  删代码而非加代码，符合本轮"还债"定位，且零性能风险。

**倾向后者**：本轮目标是还债不是加功能；把 `read_buffer_size` 变成运行时可调
是**新特性**，不是原子化。而"删掉假装能用的东西"恰恰是正交化的实质 ——
一个不生效的配置通道不是通道，是噪声。

## 现状证据（其余耦合点）

### 1. 构造入口三个，语义重叠

- `uvhttp_server_new(loop, **server)` —— `server->config` 保持 NULL
- `uvhttp_server_new_with_loop(**server)`
- `uvhttp_server_create(loop, host, port, **builder)` —— **唯一会建 config 的入口**

关键后果：`server->config` 对裸 `server_new` 用户恒为 NULL，于是
`server.c:109-120` 的 fallback 链决定行为 —— 同一 API 调用在不同装配下
结果不同。且 builder 写的 3 个字段仅在 builder 路径下存在。

### 2. 释放路径非原子：靠调用方手动置 NULL

`uvhttp_server_create` 错误回滚（`src/uvhttp_server.c:795-848`）逐步手工置
`server->config = NULL` / `server->router = NULL`。注释自承
"Do NOT null them here - that would defeat the free and leak" ——
**同一字段在相邻错误分支一会儿置一会儿不置**，正确性靠注释维护。

### 3. 所有权契约只存在于文档注释

`set_router` / `set_context` 的 `@note` 强调"server 接管所有权"，
但入参是裸指针，类型系统无表达。三处注释描述同一所有权图且互相引用不同
（`config.c:508` / `context.c:302` / `server.h:238-242`）。

### 4. 限流：API 是 per-client，实现是 per-server

| 函数 | 参数暗示 | 实际 |
|---|---|---|
| `get_rate_limit_status(server, client_ip, ...)` | 查某客户端 | 忽略 ip，返回全局计数 |
| `reset_rate_limit_client(server, client_ip)` | 重置某客户端 | 忽略 ip，注释自承 "simplified implementation: reset entire server's" |
| `check_rate_limit(server)` | —— | 无 ip 参数，全局单计数器 |

白名单却真按 IP（`request.c:358`）。**per-IP 白名单 + per-server 计数**
混在一套 API 里。A 打满 → B 一起 429。

### 5. `server` 是 6 cache line 的大 struct

限流状态、WS 连接管理、gzip cache、protocol registry 全部内联
（`#if` + `_padding` 拼布局）。四件独立的事共享一个生命周期。

## 改造原则（按 B 校准）

1. **配置只留真生效的** —— 删 14 个零读点字段 + 3 个假 setter，
   或者接线。**需用户在"删"与"接"之间拍板**（见下方待决）。
2. **单一构造入口**：`loop` 归谁由参数表达，不由函数名表达。
   builder 与其 10 个全局方法一并移除。
3. **释放原子**：失败回滚不需调用方置 NULL。
4. **所有权进类型**：接管用 `**` 或明确标注。
5. **限流语义二选一**：真 per-client，或改签名承认 per-server，
   **用语义测试锁死**。
6. **子模块独立生命周期**：限流 / TLS / WS / 压缩拆成独立对象。

## 待用户拍板（阻塞第一刀）

**`uvhttp_config_t` 那 14 个不生效的字段：删掉，还是接线？**

- 删：符合"还债不加班"定位，零性能风险，但缩减了表面能力
  （`uvhttp_set_max_body_size` 等要一并删或改语义）
- 接：是**新特性**（运行时可调 body/buffer 上限），与特性冻结矛盾，
  且动 per-connection 分配，需基准门禁验证

这是路线级选择，不是实现细节，不替他决定。

## 影响范围

- `include/uvhttp_config.h`（26 字段）、`uvhttp_server.h`（52 符号）
- `src/uvhttp_config.c`（defaults/validate/print 全部 26 字段）
- `src/uvhttp_server.c`（构造/回滚/fallback 链/限流）
- 全部测试：`test_config_*` 有大量用例断言"更新成功"，删字段需同步删
- `examples/01_basics/*` 用了 `uvhttp_config_set_current`
- `docs/` API 参考需同步；迁移说明落 README + CHANGELOG + docs/

## 关联

[[embedding-guide-examples]]、[[embedding-cmake-public-deps]]、
[[chunked-encoding-absent]]（同样"能力有但封装缺"的判断）、
[[test-validity-always-green]]、[[connection-limit-503-memory-safety]]
（该页定的原则"公共结构体字段要么是权威真源、要么不存在"——
**本发现正是该原则在 config 上的系统性违反**）


## Timeline

- time: 2026-10-09T00:27:12
  kind: decision
  summary: "Created this page: v2.10 方向：API 原子化与正交化，停止新增特性"
  source: "2026-10-09 与用户讨论确定的下个版本方向"
  affects: [api-atomization-orthogonalization]

- time: 2026-10-09T00:27:30
  kind: decision
  summary: "确立 v2.10 方向：停止新增特性，公共 API 原子化与正交化"
  source: "读 include/ 现状（299 符号 / 28 头文件）+ 追踪 server.c / request.c / config.c 实现"
  affects: [api-atomization-orthogonalization]

- time: 2026-10-09T00:39:16
  kind: decision
  summary: "兼容策略定为 B：2.10 直接破坏性改，不留 deprecated 兼容层"
  source: "用户拍板（2026-10-09）"
  affects: [api-atomization-orthogonalization]

- time: 2026-10-09T00:39:33
  kind: decision
  summary: "确定兼容策略 B：2.10 直接破坏性改造，不留兼容层"
  source: "用户拍板 2026-10-09"
  affects: [api-atomization-orthogonalization]

- time: 2026-10-09T00:43:09
  kind: evidence
  summary: "深入核实后修正：配置问题不止双通道 —— 三个 update_* 中有两个写的字段在全库无任何执行点读它们"
  source: "2026-10-09 grep 全 src 读点核实"
  affects: [api-atomization-orthogonalization]

- time: 2026-10-09T00:46:40
  kind: decision
  summary: "第一性原理重估：配置问题的真因不是双通道，而是 uvhttp_config_t 26 字段中 14 个从不被执行路径读取 —— config 是装饰品，不是控制面"
  source: "2026-10-09 逐字段读点核实（脚本按 config 对象限定匹配）"
  affects: [api-atomization-orthogonalization]
