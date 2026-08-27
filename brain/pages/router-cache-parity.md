---
id: router-cache-parity
title: "路由缓存实现必须与非缓存路由器行为对齐"
category: decision
status: active
tags: [router, cache, ci]
created: "2026-08-22T16:04:28"
updated: "2026-08-22T16:04:28"
---

<!-- compiled_truth -->
# 路由缓存实现必须与非缓存路由器行为对齐

## 编译事实

`src/uvhttp_router_cache.c` 与 `src/uvhttp_router.c` 二选一编译（由 `UVHTTP_FEATURE_ROUTER_CACHE` 控制）。两者共享同一份 `uvhttp_router_t` 公共结构和 API 契约。

## 对齐规则（PR #360 + #361 验证）

1. **公共字段同步**：`route_count`、`use_trie`、`array_route_count`、`array_capacity` 必须随 `uvhttp_router_add_route_method` 更新
2. **HYBRID_THRESHOLD=100**：非参数路由达到 100 条后 `use_trie=1` 并重置 `array_route_count`；含 `:` 的参数路由立即触发迁移
3. **array_capacity 翻倍**：满时 ×2（2→4→8…），初始为 HYBRID_THRESHOLD
4. **路径验证**：空路径 / ≥MAX_ROUTE_PATH_LEN(256) / 含 `?` → `UVHTTP_ERROR_INVALID_PARAM`
5. **静态前缀**：`uvhttp_router_match` 仅在 array 模式（`!use_trie`）检查；`uvhttp_router_find_handler` 始终检查并回退 fallback
6. **参数化匹配**：hash 表二遍扫描支持 `/items/:id` 段级匹配；`uvhttp_router_match` 通过模板对比提取参数名值

## 测试策略

- `test_router_boost_coverage.cpp` 同时覆盖两种实现（不再文件级禁用）
- 仅 trie 内部结构断言（如 node_pool_size）用 `#if !UVHTTP_FEATURE_ROUTER_CACHE` 守卫
- build-matrix 的 router-cache job 必须跑完整 ctest（101 tests），不允许 run-tests:false

## 教训

用 `#if` 禁用整个测试文件来"修复"CI 是掩盖失败——重新启用后暴露了 9 个真实 bug（路径验证缺失、容量逻辑错误、阈值迁移缺失、静态前缀语义错误）。


## Timeline

- time: 2026-08-22T16:04:28
  kind: decision
  summary: "Created this page: 路由缓存实现必须与非缓存路由器行为对齐"
  source: created via brain create-page
  affects: [router-cache-parity]

- time: 2026-08-22T16:04:28
  kind: decision
  summary: "ROUTER_CACHE=ON 的 cache_optimized_router_t 是 uvhttp_router_t 的替代实现，公共结构体字段与 API 行为必须与非缓存路由器逐项对齐，否则测试套件会暴露差异"
  source: brain update-truth
  affects: [router-cache-parity]
