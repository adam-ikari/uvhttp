# uvhttp 路由查找模式配置指南

## 📋 概述

uvhttp 提供了三种路由查找模式，可根据应用场景和性能需求进行选择：

- **模式 0**: 纯线性查找 - 适用于小规模应用
- **模式 1**: 纯哈希查找 - 适用于中等规模应用  
- **模式 2**: 混合策略 - 适用于大规模高并发应用（默认）

## ⚙️ 编译配置

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

## 📊 模式对比

| 模式 | 内存开销 | 查找性能 | 适用场景 | 推荐配置 |
|------|---------|---------|---------|---------|
| 0 - 线性 | 最小 | O(n) | 嵌入式、<10路由 | `SEARCH_MODE=0` |
| 1 - 哈希 | 中等 | O(1) | 中等规模、10-50路由 | `SEARCH_MODE=1` |
| 2 - 混合 | 较大 | O(1)最优 | 大规模、>50路由 | `SEARCH_MODE=2` |

## 🎯 场景推荐

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

## 🔧 性能调优

### 内存优化
- 使用 `SEARCH_MODE=0` 减少内存占用
- 禁用缓存优化节省 4KB+ 内存

### 性能优化
- 使用 `SEARCH_MODE=2` 获得最佳查找性能
- 启用统计功能监控性能表现
- 使用 `-O3 -march=native` 优化编译

### 调试友好
- 使用 `SEARCH_MODE=0` 简化调试过程
- 禁用复杂缓存逻辑

## 📈 性能基准

理论性能估算（50个路由）：

| 模式 | 平均查找时间 | 内存开销 | 缓存命中率 |
|------|-------------|---------|-----------|
| 线性 | ~1500ns | 0KB | N/A |
| 哈希 | ~200ns | ~4KB | N/A |
| 混合 | ~180ns | ~8KB | ~60% |

## 🚀 最佳实践

1. **开发阶段**: 使用 `SEARCH_MODE=0` 便于调试
2. **测试阶段**: 使用 `SEARCH_MODE=1` 验证功能
3. **生产环境**: 使用 `SEARCH_MODE=2` 保证性能
4. **嵌入式**: 始终使用 `SEARCH_MODE=0`
5. **高并发**: 使用 `SEARCH_MODE=2` + 统计功能

---

*基于 uvhttp 高性能 HTTP 服务器设计，为不同场景提供最优的路由查找策略。*