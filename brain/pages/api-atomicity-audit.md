---
id: api-atomicity-audit
title: "API 原子化审计：按原子性判据逐条定位违约点（不考虑兼容）"
category: decision
status: active
tags: [api, atomicity, orthogonality, audit, v2.10]
created: "2026-10-09T00:49:13"
updated: "2026-10-09T01:08:06"
---

<!-- compiled_truth -->
# API 原子化与正交性审计

配套 [[api-atomization-orthogonalization]]（方向与兼容策略 B）。
本页按**用户给定的定义**审计，明确排除兼容性与迁移成本。

## 判据（用户定义，2026-10-09）

- **原子性 = API 不可再分割。** 一个 API 若能拆成若干独立子步骤、
  由调用方按序拼装才完成一件事，则它不是原子的。判据形式化：
  **完成"一件事"的最小调用数 > 1，且子步骤之间无强制约束表达。**
- **正交性 = API 没有重合。** 两个 API 若做同一件事、或一个的能力
  是另一个的真子集，则它们不正交。判据形式化：
  **存在两个 API f、g，使得 g 能表达的 f 都能表达，且无一方是必要的。**

> ⚠️ **本页此前用过一套错误判据**（C1 提交性 / C2 顺序无关性 / C3 幂等 /
> C4 无跨对象半状态）——那是**事务原子性**，与此处定义无关，已作废。
> 见本页 timeline 的 reversal 条目。原判据下的发现中，
> 仍然成立的部分（A2 router 数据丢失、A4 配置静默失效）是**独立缺陷**，
> 不依赖那套判据 —— 它们是功能 bug，不是原子性问题。
> 本次审计**不重复**它们，只做不可分割性与重合性的定位。

## 一、原子性：不可再分割

### N1 构造一个可用的 server = 5 步 ★最严重的不可分割

官方嵌入示例 `examples/embedding/main.c:60-100` 的实证：

```c
uvhttp_server_new(loop, &server);              // 1
uvhttp_router_new(&router);                    // 2
uvhttp_router_add_route(router, "/", h);       // 3
uvhttp_server_set_router(server, router);      // 4
uvhttp_server_listen(server, "0.0.0.0", port); // 5
```

"起一个监听 8080、带一条根路由的服务器"是**一件事**，但 API 把它拆成 5 个
可独立调用的步骤。更严重的是**这 5 步没有原子版本**：`server_create`
虽然一步建好，但它建的是 builder 而非 server，且**不带路由**。

代价直接可见 —— 示例里手写了 **4 个错误回滚分支**：

```c
if (router_new 失败)  { server_free(server); return 1; }
if (add_route 失败)   { router_free(router); server_free(server); return 1; }
if (listen 失败)      { router_free(router); server_free(server); return 1; }
```

第 3、4 分支还要注意：**`set_router` 之后 router 已归 server 所有**，
但 listen 失败分支仍调 `router_free(router)` —— 与
`set_router` 的 `@note`（"the server owns the router ... Do NOT call
uvhttp_router_free on it yourself"）**直接冲突**。

**官方示例就在教违反所有权契约的写法。** 这是不可分割性缺陷最尖锐的表现：
调用方被迫知道"哪一步之后所有权转移了"，而这个知识没有任何类型或 API 表达。

**原子形态应该是**：`uvhttp_server_listen_with(server, host, port, routes, n)`
一类，或者构造器接受路由表 —— 要么全成，要么不成。

### N2 发送一个响应 = 4 步，且无原子版本

```c
uvhttp_response_set_status(response, 200);
uvhttp_response_set_header(response, "Content-Type", "text/plain");
uvhttp_response_set_body(response, body, len);
return uvhttp_response_send(response);
```

（`examples/01_basics/helloworld.c:111-127` 实证）

这 4 步之间**没有任何约束**：`send` 一个没有 body 的响应会怎样？
`set_body` 两次会怎样（第二次释放第一次，见 A3）？`set_status` 不调直接
`send` 会怎样（发 0 或 200）？API 不表达这些，行为由实现默认值决定。

`uvhttp_response_init` 还在前面（`response.h:100`），完整序列 5 步。

**原子形态应该是**：`uvhttp_respond(response, status, content_type, body, len)`。

### N3 配置生效 = 3 步且通道二选一

调用方要弄清：设 `server->config` 还是 `context->current_config`？
`server.c:109-120` 的 fallback 链决定生效值，而走哪条取决于用了哪个构造
入口。**"设置一个配置项"这件事被拆成"选通道 + 装配 + 设置"三步，
且通道选择不可见。**

### N4 请求读取：命名暗示两个对象，实为一个

`uvhttp_get_header(request, name)` 声明在 **`uvhttp_server.h`**（`server.h:413-415`），
操作的是 `uvhttp_request_t`。三个函数全是纯转发：

```c
const char* uvhttp_get_header(request, name) {
    return uvhttp_request_get_header(request, name);   // 纯别名
}
const char* uvhttp_get_param(request, name) {
    return uvhttp_request_get_query_param(request, name);
}
const char* uvhttp_get_body(request) {
    return uvhttp_request_get_body(request);
}
```

放在 `server.h` 里**没有理由** —— 它们不涉及 server。这本身就是
"归属错误"的信号：API 的位置暗示了一个它并不拥有的对象。

## 二、正交性：没有重合

### O1 请求读取 API 双份并存 ★纯粹的重合

| 短名（`server.h:413-415`） | 正规名（`request.h:107-109`） |
|---|---|
| `uvhttp_get_header(req, name)` | `uvhttp_request_get_header(req, name)` |
| `uvhttp_get_param(req, name)` | `uvhttp_request_get_query_param(req, name)` |
| `uvhttp_get_body(req)` | `uvhttp_request_get_body(req)` |

**三对全部是别名**，短名版函数体只有一行转发。同一能力两个入口，
签名与行为完全一致。**选哪个没有任何依据** —— 不是简写、不是废弃、
不是不同层次。纯粹的重复。

而且短名版有信息损失：`uvhttp_get_param` 实际是 **query** param
（转发到 `get_query_param`），名字里的 "param" 有歧义（路由参数 `:id`
才是通常说的 param）。**重合且其中一个名字是错的。**

### O2 两个 cache API 结构完全同构，仅类型名不同

`uvhttp_lru_cache` 与 `uvhttp_gzip_cache` 是**同一个抽象的两份实现**：

| 操作 | LRU | gzip |
|---|---|---|
| create | `lru_cache_create(max_mem, max_entries, ttl, **cache)` | `gzip_cache_create(max_mem, max_entries, ttl, **cache)` |
| free | `lru_cache_free(cache)` | `gzip_cache_free(cache)` |
| put | `lru_cache_put(...)` | `gzip_cache_put(...)` |
| stats | `lru_cache_get_stats(...)` | `gzip_cache_get_stats(...)` |
| set_max_entries | `lru_cache_set_max_entries(cache, n)` | `gzip_cache_set_max_entries(cache, n)` |
| set_max_memory_usage | 同名同参 | 同名同参 |
| set_cache_ttl | 同名同参 | 同名同参 |

**签名逐字相同。** 若把类型名参数化，这两套可以合并为一个泛型 cache。
现在的形态迫使调用方为两种缓存写两遍几乎相同的代码。

配套的不一致：类型名 `cache_manager_t`（**无 `uvhttp_` 前缀**，
`lru_cache.h:24`）vs `uvhttp_gzip_cache_t`（有前缀）。同一层抽象的
两个类型，一个遵守命名约定一个不遵守。

**正交化含义**：cache 应该是一个抽象（接口 + 单一实现），
gzip cache 是它的一个策略选择，而不是两个平行 API。

### O3 构造入口三重叠

| 入口 | 建 config | 建 router | listen | 返回 |
|---|---|---|---|---|
| `server_new(loop, **s)` | ✗ | ✗ | ✗ | server |
| `server_new_with_loop(**s)` | ✗ | ✗ | ✗ | server（loop 归它管）|
| `server_create(loop, host, port, **b)` | ✓ | ✓ | **✓** | builder |

三者不是"不同功能"，而是**同一件事的三种切分**。`server_create` 做的
前两步 `server_new` 也能做，差别只是**顺序**。这是 N1 不可分割的
另一面：因为没有原子构造，才需要多个非原子入口拼。

### O4 释放语义四种写法，同义多名

| 写法 | 返回 | 出现处 |
|---|---|---|
| `uvhttp_server_free` | `uvhttp_error_t` | `server.h:246` |
| `uvhttp_server_simple_free` | `void` | `server.h:428` |
| `uvhttp_response_free` | `void` | `response.h:303` |
| `uvhttp_response_cleanup` | `void` | `response.h:302` |
| `uvhttp_request_free` | `void` | `request.h:100` |
| `uvhttp_request_cleanup` | `void` | `request.h:101` |

`free` / `cleanup` / `simple_free` 三组同义命名并存，**没有任何文档说明
区别**。`response_free` 与 `response_cleanup` 同一个头文件里相隔一行。
返回类型也不一致（一个 `uvhttp_error_t`，其余 `void`）。

**这是重合**：三个名字指同一件事，说明其中两个是多余的。

### O5 压缩控制五入口，部分互相包含

`uvhttp_response_set_compress` / `set_compress_algorithm` /
`set_compress_threshold` / `set_compress_by_filename` /
`set_compress_by_content_type`，加上两个判定辅助
`uvhttp_should_compress_by_extension` / `uvhttp_should_compress_by_content_type`
（`response.h:217,248`）。

后两个 `should_compress_*` 是**纯函数判定**，前两个 `set_compress_by_*`
是**设置 + 内部调用判定**。同一套扩展名/Content-Type 匹配逻辑
（`response.c:1183` `COMPRESSIBLE_EXTENSIONS` 与
`NON_COMPRESSIBLE_EXTENSIONS` 两张表）被判定和设置两条路径共用，
但两条路径都暴露给调用方。**判定逻辑与设置动作没有分开。**

## 三、按"不可分割 / 无重合"归并的结论

| 维度 | 现状 |
|---|---|
| 构造一件事 | 5 步，无原子版本，官方示例手写 4 个回滚分支且违反所有权契约 |
| 发一个响应 | 4~5 步，无原子版本，步骤间无约束 |
| 设一个配置项 | 3 步 + 通道二选一，通道不可见 |
| 读一个 header | 2 个入口，纯别名，其中一个名字有歧义 |
| 用一个 cache | 2 套逐字相同的 API，仅类型名不同 |
| 构造 server | 3 个入口，差异只是切分顺序 |
| 释放 | 3 组同义命名，4 种返回类型 |
| 控制压缩 | 5 个入口 + 2 个判定辅助，判定与设置未分离 |

**一句话**：这个 API 的问题不是"不安全"或"有 bug"，而是**它把每件事
都拆成了可独立调用的碎片，然后把组合的责任交给了调用方**。
调用方要读注释才能知道步骤顺序、要读注释才知道所有权何时转移、
要读注释才知道哪个入口是"正确"的那个。

这解释了为什么 [[connection-limit-503-memory-safety]] 那类"字段要么权威
要么不存在"的原则难以贯彻 —— 当 API 本身是碎片化的，字段状态就没有
唯一可以依附的地方。

## 四、正交化已达成的维度（不要破坏）

- **无跨实例可变全局状态**。全库仅 4 个 `static const` 表；
  TLS entropy/drbg 挂在 `context` 上。多 server 隔离天然成立。
- **response 系 setter 先校验后写入**，不变量在写入点建立
  （`set_status` 验范围、`set_header` 验字符与控制符）。
  这是 cache 系 setter（直接赋值无校验）应统一的模板。

## 五、待定（不属本次审计范围）

- `uvhttp_config_t` 26 字段中 14 个不生效 —— 属功能缺陷，
  见 [[api-atomization-orthogonalization]]，不重复。
- router 迁移致路由全丢（13+ 顶层路由 + 参数路由）—— 属数据丢失 bug，
  已实测复现，见 [[api-atomicity-audit]] timeline 的 evidence 条目。

## 关联

[[api-atomization-orthogonalization]]（方向、兼容策略 B、配置字段全表）、
[[embedding-guide-examples]]（N1 不可分割的实证来源 —— 官方示例）、
[[router-cache-parity]]（O2 同构双缓存的既有认知）、
[[connection-limit-503-memory-safety]]（碎片化 API 如何导致字段无唯一依附点）


## Timeline

- time: 2026-10-09T00:49:13
  kind: decision
  summary: "Created this page: API 原子化审计：按原子性判据逐条定位违约点（不考虑兼容）"
  source: "2026-10-09 按原子性判据审计全量公共 API"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:49:47
  kind: decision
  summary: "原子化判据与 12 条违约点定位：构造非原子、迁移无回滚、set_body 先释放后分配、无全局状态（唯一干净维度）"
  source: "2026-10-09 逐函数审计 src/ 实现"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:52:29
  kind: evidence
  summary: "A2 实测确认（比原判断更严重）：13 个不同顶层路由 + 任意参数路由 → migrate_to_trie 中途失败 → 全部路由丢失且 find_handler 返回 NULL，无任何报错"
  source: "2026-10-09 独立探针程序实测（4 个 probe，逐个排除后定位）"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:52:56
  kind: decision
  summary: "A2 实测升级为已复现的数据丢失：13+ 顶层路由 + 参数路由致全部路由静默丢失，附机制解剖与修复方向"
  source: "2026-10-09 探针实测 + 机制追至 child_indices[12] 定长上限"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:53:01
  kind: reversal
  summary: "修正早前判断：初版称 A2 仅『面临丢失风险』且迁移后新路由缺失，量级判断不足 —— 实测为已注册路由全部丢失"
  source: "2026-10-09 探针实测推翻"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:56:28
  kind: decision
  summary: "按用户定义重写：原子性=API 不可再分割，正交性=API 无重合。原 C1-C4 事务语义判据作废"
  source: "2026-10-09 用户纠正判据定义后重写"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:56:32
  kind: reversal
  summary: "判据定义作废重写：此前用的 C1-C4（提交性/顺序无关/幂等/跨对象半状态）是事务原子性，与用户所指（API 不可再分割）无关"
  source: "2026-10-09 用户纠正定义"
  affects: [api-atomicity-audit]

- time: 2026-10-09T01:08:06
  kind: evidence
  summary: "N1 的实证升级：examples/embedding/main.c 的 listen 失败回滚分支在 ASan 下确认 double-free（set_router 已转移所有权，示例仍 router_free）"
  source: "2026-10-09 ASan 探针复现，栈指向 uvhttp_server_free→uvhttp_router_free"
  affects: [api-atomicity-audit]
