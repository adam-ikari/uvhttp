# uvhttp 路由查找模式配置指南

## 概述

uvhttp 的路由查找有三种模式，区别在用什么结构存路由。模式 0 纯线性查找，路由少的时候够用；模式 1 纯哈希查找，适合中等规模；模式 2 混合策略，面向大规模高并发场景，也是默认值。

## 编译配置

### 基本用法

```bash
# 默认混合策略
gcc ...

# 纯线性查找（嵌入式/小规模应用）
gcc -DUVHTTP_ROUTER_SEARCH_MODE=0 ...

# 纯哈希查找（中等规模应用）
gcc -DUVHTTP_ROUTER_SEARCH_MODE=1 ...

# 混合策略（大规模应用）
gcc -DUVHTTP_ROUTER_SEARCH_MODE=2 ...
```

### 相关配置选项

```bash
# 禁用路由缓存优化
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION=0 ...

# 启用统计功能
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 ...

# 启用动态调整
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC=1 ...

# 启用性能监控
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_MONITORING=1 ...
```

## 模式对比

| 模式 | 内存开销 | 查找性能 | 适用场景 | 推荐配置 |
|------|---------|---------|---------|---------|
| 0 - 线性 | 最小 | O(n) | 嵌入式、<10路由 | `SEARCH_MODE=0` |
| 1 - 哈希 | 中等 | O(1) | 中等规模、10-50路由 | `SEARCH_MODE=1` |
| 2 - 混合 | 较大 | O(1)最优 | 大规模、>50路由 | `SEARCH_MODE=2` |

## 场景推荐

### 嵌入式设备
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=0 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION=0 \
     -Os -DCONFIG_SMALL_MEMORY
```

### 微服务架构
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=1 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 \
     -O2
```

### 高并发服务器
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=2 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC=1 \
     -O3 -march=native
```

## 性能调优

### 内存优化

内存吃紧时把查找模式切到 `SEARCH_MODE=0`，线性表比哈希表省空间。再关掉缓存优化，还能省下 4KB+ 内存。

### 性能优化

追求查找速度就用 `SEARCH_MODE=2`。统计功能打开后可以看到缓存命中情况，判断当前配置是否合适。编译时加 `-O3 -march=native` 让编译器针对本机指令集优化。

### 调试友好

调试阶段用 `SEARCH_MODE=0`，匹配过程就是顺序遍历，出了问题容易跟。复杂的缓存逻辑这时可以关掉。

## 性能基准

理论性能估算（50个路由）：

| 模式 | 平均查找时间 | 内存开销 | 缓存命中率 |
|------|-------------|---------|-----------|
| 线性 | ~1500ns | 0KB | N/A |
| 哈希 | ~200ns | ~4KB | N/A |
| 混合 | ~180ns | ~8KB | ~60% |

## 最佳实践

1. 开发阶段用 `SEARCH_MODE=0`，调试时能看清每一次匹配。
2. 测试阶段用 `SEARCH_MODE=1`，验证哈希路径的功能是否正确。
3. 生产环境用 `SEARCH_MODE=2`，兼顾查找速度和规模。
4. 嵌入式设备固定用 `SEARCH_MODE=0`，内存比速度更紧张。
5. 高并发服务用 `SEARCH_MODE=2`，同时打开统计功能观察命中率。

---

*这份配置说明来自 uvhttp 的 HTTP 服务器实现，按部署规模给出对应的查找策略。*