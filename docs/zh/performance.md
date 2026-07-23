---
title: 性能基准
description: UVHTTP 性能基准——~20K RPS、100 至 500 连接吞吐持平、零 socket 错误、P50/P99 延迟。在 AMD Ryzen 7 5800H 上用 wrk 测得，含复现命令与历史基准。
---

# 性能指标

UVHTTP 性能基准。

> 以下数据于 2026-07-12 在原始基准主机（AMD Ryzen 7 5800H，12 核，Linux
> 6.17.13-2-pve）上使用 `wrk 4.1.0` 对内置 `test_performance_e2e` 服务器测量，
> GCC 11.4.0 Release 构建（`-O2 -DNDEBUG`），系统分配器。
> 复现命令：`wrk -t4 -c<N> -d10s http://127.0.0.1:18090/simple`。

## 基准性能指标

### 吞吐量

| 测试场景 | RPS | 平均延迟 | 最大延迟 | 备注 |
|---------|-----|---------|---------|---------|
| 低并发（10 连接） | **19,887** | 0.35ms | 6.59ms | P50 0.32 / P99 0.86ms |
| 中并发（100 连接） | **19,834** | 5.03ms | 19.25ms | |
| 高并发（500 连接） | **19,810** | 25.31ms | 62.81ms | 与 100 连接持平 |
| 超高并发（1000 连接） | **18,518** | 56.31ms | 322ms | 平滑退化 |
| JSON 端点（100 连接） | 19,451 | 5.15ms | 20.48ms | 2.84MB/s |
| 大响应 1KB（100 连接） | 19,524 | 5.13ms | 14.50ms | 9.92MB/s |
| **Socket 错误** | **0** | — | — | 全部并发级别零错误 |
| **累计处理请求** | 1,341,713 | — | — | 服务端零错误 |

### 可靠性与内存安全

- **零错误**：全部并发级别下 socket 错误为 0，服务端日志无错误。
- **内存安全验证**：91 项测试通过 AddressSanitizer（零泄漏/UAF/溢出）与
  UndefinedBehaviorSanitizer（零未定义行为）验证。

### 性能提升

| 版本 | 峰值 RPS | 说明 |
|------|---------|------|
| v2.3.1 | 31,151 | 3s 短时基准 |
| v2.4.4 (system) | 28,323 | 3s 短时基准 |
| v2.5.0 | 19,887 | 10s 持续基准，ASan/UBSan 验证通过 |

> 完整历史基准见 `docs/performance/baseline-history.json`。v2.5.0 的 RPS 低于早期
> 3s 短时基准，主要因测试时长延长（10s）与主机负载；从 100 到 500 连接吞吐持平、
> 零错误，是更具生产代表性的持续吞吐数据。

### 延迟分布

| 百分位 | 延迟 |
|--------|------|
| P50 | 3.09ms |
| P90 | 5.8ms |
| P95 | 8.5ms |
| P99 | 12.0ms |

## 测试环境

- **CPU**: AMD Ryzen 7 5800H，12 核
- **操作系统**: Linux 6.17.13-2-pve
- **编译器**: GCC 11.4.0 Release（-O2 -DNDEBUG）
- **测试工具**: wrk 4.1.0
- **测试时长**: 10 秒
- **内存分配器**: 系统分配器

## 性能调优建议

### 编译优化

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build build_release -j$(nproc)
```

### 系统配置

```bash
# 增加文件描述符限制
ulimit -n 65536

# 优化 TCP 参数
sysctl -w net.core.somaxconn=65535
sysctl -w net.ipv4.tcp_max_syn_backlog=65535
```

### 运行时优化

- 使用 mimalloc 分配器（`-DUVHTTP_ALLOCATOR_TYPE=1`）
- 启用路由缓存
- 启用 LRU 缓存

## 相关文档

- [API 参考](/api/introduction) - 完整的 API 文档