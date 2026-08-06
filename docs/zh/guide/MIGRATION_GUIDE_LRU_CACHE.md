---
title: LRU 缓存迁移指南
---
# LRU 缓存迁移指南

## 概述

本迁移指南帮助你将代码更新为使用 UVHTTP 2.3.0 引入的简化版 LRU 缓存 API。

## 破坏性变更

### 已移除的函数

以下函数已被移除：

1. `uvhttp_lru_cache_set_eviction_mode()` - 淘汰模式选择
2. `uvhttp_lru_cache_init_task_queue()` - 任务队列初始化
3. `uvhttp_lru_cache_schedule_eviction()` - 调度淘汰
4. `uvhttp_lru_cache_stop_task_queue()` - 停止任务队列
5. `uvhttp_lru_cache_perform_eviction()` - 执行淘汰

### 已移除的特性

- **LFU 淘汰模式**: 最少使用频率淘汰
- **混合淘汰模式**: LRU/LFU 组合淘汰
- **双阈值淘汰**: 80%/95% 淘汰阈值
- **任务队列机制**: 通过 uv_async_t 实现的异步淘汰

## 迁移步骤

### 步骤 1：移除淘汰模式配置

**迁移前:**
```c
uvhttp_lru_cache_set_eviction_mode(cache, UVHTTP_CACHE_EVICTION_MODE_LFU);
```

**迁移后:**
```c
// 无需任何操作 - LRU 现在是唯一的淘汰模式
```

### 步骤 2：移除任务队列初始化

**迁移前:**
```c
uvhttp_lru_cache_init_task_queue(cache, loop);
```

**迁移后:**
```c
// 无需任何操作 - 任务队列机制已移除
```

### 步骤 3：移除淘汰调度

**迁移前:**
```c
uvhttp_lru_cache_schedule_eviction(cache);
```

**迁移后:**
```c
// 无需任何操作 - 淘汰现在是同步且自动的
```

### 步骤 4：移除任务队列清理

**迁移前:**
```c
uvhttp_lru_cache_stop_task_queue(cache);
```

**迁移后:**
```c
// 无需任何操作 - 已无任务队列需要停止
```

## 新行为

### 单阈值淘汰

缓存现在使用单一阈值（90%）替代双阈值（80%/95%）。

**迁移前:**
- 容量达到 80% 时开始淘汰
- 容量达到 95% 时进行激进淘汰

**迁移后:**
- 容量达到 90% 时开始淘汰
- 批量大小：2 个条目（可通过 `uvhttp_lru_cache_set_batch_eviction_size()` 配置）

### 同步淘汰

淘汰现在在请求路径中同步执行，消除了任务队列的开销。

**迁移前:**
```c
// 通过任务队列进行异步淘汰
uvhttp_lru_cache_schedule_eviction(cache);
```

**迁移后:**
```c
// 自动同步淘汰
// 无需手动调度
```

### 简化配置

**迁移前:**
```c
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_create(1000, 3600);
uvhttp_lru_cache_set_eviction_mode(cache, UVHTTP_CACHE_EVICTION_MODE_LRU);
uvhttp_lru_cache_init_task_queue(cache, loop);
```

**迁移后:**
```c
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_create(1000, 3600);
// 缓存立即可用
```

## 配置选项

### 批量淘汰大小

你可以配置每批次淘汰的条目数量：

```c
// 设置批量淘汰大小（默认：2）
uvhttp_lru_cache_set_batch_eviction_size(cache, 5);
```

### 缓存 TTL

设置缓存条目的存活时间：

```c
// 设置缓存 TTL（默认：3600 秒）
uvhttp_lru_cache_set_cache_ttl(cache, 7200);
```

## 性能影响

### 内存节省

- **实例开销**: 每个缓存实例减少 132 字节
- **条目开销**: 每个缓存条目减少 4 字节
- **代码量**: 减少 200 行代码

### 性能提升

- **零 libuv 依赖**: 无异步句柄或任务队列
- **更简单的淘汰逻辑**: 更快的淘汰决策
- **更低的开销**: 无任务队列调度

## 测试

更新你的测试，移除对已移除函数的调用：

```c
// 移除这些测试用例
TEST(LRUCacheTest, EvictionMode) { /* ... */ }
TEST(LRUCacheTest, TaskQueue) { /* ... */ }

// 添加新的测试用例
TEST(LRUCacheTest, BatchEvictionSize) { /* ... */ }
TEST(LRUCacheTest, SingleThresholdEviction) { /* ... */ }
```

## 兼容性矩阵

| UVHTTP 版本 | 淘汰模式 | 任务队列 | 是否需要迁移 |
|----------------|----------------|------------|-------------------|
| 2.2.x | LRU, LFU, 混合 | 是 | - |
| 2.3.0 | 仅 LRU | 否 | 是 |

## 回滚方案

如果需要回滚到 UVHTTP 2.2.x：

1. 恢复已移除的函数调用
2. 重新添加淘汰模式配置
3. 重新添加任务队列初始化
4. 更新版本依赖

## 支持

如果在迁移过程中遇到问题：

1. 查看 [API 参考](../../api/API_REFERENCE.md)
2. 浏览 [示例](https://github.com/adam-ikari/uvhttp/tree/main/examples)
3. 在 [GitHub](https://github.com/adam-ikari/uvhttp/issues) 上提交 issue

## 总结

简化版 LRU 缓存 API 提供：

- ✅ **更简单的配置**: 无需选择淘汰模式
- ✅ **更好的性能**: 无任务队列开销
- ✅ **更少的内存**: 降低内存占用
- ✅ **更易维护**: 更少的代码路径

对于大多数用例，迁移过程非常简单——只需移除对已弃用函数的调用即可。
