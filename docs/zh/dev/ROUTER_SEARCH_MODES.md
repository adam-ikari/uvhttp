# uvhttp 路由查找模式配置指南

## 概述

路由查找方式在编译期由单个 CMake 选项决定：`BUILD_WITH_ROUTER_CACHE`，默认 `OFF`。它把 `UVHTTP_FEATURE_ROUTER_CACHE` 定义为 0 或 1，只编译其中一份实现：

| 取值 | 源文件 | 路由存储 |
|------|--------|----------|
| `OFF` (0) | `src/uvhttp_router.c` | 路由数组，满足条件后迁移到 Trie |
| `ON` (1) | `src/uvhttp_router_cache.c` | 开放寻址哈希表 |

没有运行时开关，也没有单独的路由查找模式宏。

## 编译配置

```bash
# 默认：数组 + Trie
cmake -B build -DBUILD_WITH_ROUTER_CACHE=OFF ..

# 哈希表实现
cmake -B build -DBUILD_WITH_ROUTER_CACHE=ON ..
```

## 路由存储方式

### 数组 + Trie（`BUILD_WITH_ROUTER_CACHE=OFF`）

- `uvhttp_router_new` 分配 `HYBRID_THRESHOLD`（100）个 `array_route_t` 槽位和 64 个节点的 Trie 节点池。
- 路由处于数组模式时追加到数组；查找时顺序扫描数组，比较路径和方法。
- 路径含 `:`（路径参数）、`array_route_count` 达到 `HYBRID_THRESHOLD`，或 `use_trie` 已置位时，选择 Trie 路径。首次切换由 `migrate_to_trie` 把所有数组路由复制进 Trie，新路由随后也加入 Trie。
- 迁移是全有或全无。失败时 `use_trie` 保持 0，已有路由继续可用，触发迁移的这条路由返回错误。
- Trie 查找按路径段逐层下探；参数段匹配任意值。

### 哈希表（`BUILD_WITH_ROUTER_CACHE=ON`）

- 所有路由存于一张开放寻址、线性探测的哈希表。表项以路径加方法为键；哈希只由路径经 XXH64 计算。
- 表初始为 `UVHTTP_ROUTER_HASH_BASE_SIZE`（256）个槽位；元素数达到容量的 `UVHTTP_ROUTER_HASH_LOAD_FACTOR`（0.75）时翻倍，上限为 `UVHTTP_ROUTER_HASH_MAX_SIZE`（4096）。
- 精确路径按哈希匹配；含 `:` 的参数化路由通过第二次线性遍历表项匹配。
- `uvhttp_router_add_route_method` 执行同样的阈值判断，在 100 条路由或首条参数化路由时置 `use_trie`，使公开的 `uvhttp_router_t` 字段与数组实现保持一致。这份实现没有 Trie，查找始终走哈希表。

## 阈值与实测开销

`HYBRID_THRESHOLD` 是各实现文件内的常量（两份实现都是 100），不是编译选项。要改交叉点，直接改源码并重跑基准。

`src/uvhttp_router.c` 中记录的耗时（1000 次查找，数组对比 Trie）：

| 路由数 | 数组 | Trie |
|--------|------|------|
| 50 | 0.02 ms | 0.03 ms |
| 100 | 0.04 ms | 0.04 ms |
| 200 | 0.08 ms | 0.05 ms |
| 500 | 0.20 ms | 0.06 ms |

哈希表实现没有记录对应的耗时数据。

## 如何选择

- 静态路由少于 100 条：保持 `OFF`。数组扫描有竞争力（50 条路由的实测更快），内存由预分配的数组和节点池封顶。
- 100 条及以上，或含参数化路由：`OFF` 用 Trie，查找开销随路径深度增长；`ON` 的精确路径查找平均 O(1)，参数化路由走线性扫描。
- 内存：`OFF` 预分配 100 个数组槽位加 64 节点池。`ON` 初始为 256 槽哈希表，每个槽位带完整的 256 字节路径缓冲，低路由数时占用更大，并随路由增加动态扩容。
- 该选项对整个构建生效，不能按请求切换。
