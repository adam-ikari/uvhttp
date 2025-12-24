# UVHTTP 压力测试综合报告

**测试时间:** Wed 24 Dec 2025 10:43:29 PM CST  
**测试环境:** Linux server-002 6.14.11-2-pve #1 SMP PREEMPT_DYNAMIC PMX 6.14.11-2 (2025-09-12T09:46Z) x86_64 GNU/Linux  
**CPU核心数:** 12  
**总内存:** 12Gi  

## 测试概览

本报告包含UVHTTP服务器的全面压力测试结果，涵盖以下测试项目：

1. **并发连接测试** - 测试服务器在高并发连接下的表现
2. **吞吐量测试** - 测量每秒请求数(RPS)性能
3. **内存泄漏测试** - 长时间运行的内存稳定性
4. **边界条件测试** - 极限负载下的系统行为
5. **性能基准测试** - 系统基础性能指标

## 详细结果

### 性能基准测试

```
timeout: failed to run command ‘./test_performance_benchmark’: No such file or directory
```

### 简单压力测试

```
timeout: failed to run command ‘./test_simple_stress’: No such file or directory
```


## 测试结论

请参考上述详细结果分析UVHTTP服务器的性能表现和稳定性。

## 文件位置

- 详细结果文件: `stress_test_results_20251224_224327/`
- 本报告: `stress_test_results_20251224_224327/stress_test_summary.md`

---
*报告生成时间: Wed 24 Dec 2025 10:43:29 PM CST*
