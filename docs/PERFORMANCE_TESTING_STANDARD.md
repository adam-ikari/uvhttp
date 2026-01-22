# UVHTTP 基准性能测试标准

## 文档信息

- **版本**: 1.0.0
- **创建日期**: 2026-01-09
- **最后更新**: 2026-01-09
- **维护者**: UVHTTP 开发团队
- **状态**: 正式发布

## 1. 目的

本文档定义了 UVHTTP 项目基准性能测试的标准规范，确保所有性能测试在一致的条件下进行，保证测试结果的可靠性、可重复性和可比性。

## 2. 适用范围

本标准适用于：
- UVHTTP 核心库的性能基准测试
- 各版本间的性能回归测试
- 新功能的性能验证测试
- 性能优化效果评估

## 3. 测试环境标准

### 3.1 硬件要求

#### 最低配置
- **CPU**: 4 核，2.0 GHz 及以上
- **内存**: 8GB 及以上
- **网络**: 1Gbps 或更高
- **磁盘**: SSD（推荐）

#### 推荐配置
- **CPU**: 8 核，3.0 GHz 及以上
- **内存**: 16GB 及以上
- **网络**: 1Gbps 或更高
- **磁盘**: NVMe SSD

#### 参考测试环境
```
操作系统: Linux 6.14.11-2-pve
CPU: AMD Ryzen 7 5800H (16核，8核在线)
内存: 12GB (11GB可用)
```

### 3.2 软件要求

#### 操作系统
- **推荐**: Linux (Ubuntu 20.04+, CentOS 8+, Debian 11+)
- **内核版本**: 5.4 或更高

#### 编译器
- **GCC**: 9.0 或更高
- **Clang**: 10.0 或更高

#### 依赖库
- **libuv**: 1.40.0 或更高
- **mbedtls**: 2.28.0 或更高
- **mimalloc**: 最新稳定版（推荐用于生产测试）

### 3.3 系统配置

#### CPU 性能模式
```bash
# 设置为性能模式
sudo cpupower frequency-set -g performance

# 验证 CPU 频率
cpupower frequency-info
```

#### 内存配置
```bash
# 禁用 swap（推荐）
sudo swapoff -a

# 验证 swap 状态
free -h
```

#### 网络配置
```bash
# 增加本地连接限制
sudo sysctl -w net.core.somaxconn=65535
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=8192
```

#### 文件描述符限制
```bash
# 增加文件描述符限制
ulimit -n 65535

# 验证
ulimit -n
```

## 4. 测试工具标准

### 4.1 必需工具

#### wrk（主要测试工具）
- **版本**: 4.2.0 或更高
- **安装**: `sudo apt-get install wrk` 或从源码编译
- **用途**: HTTP 压力测试

#### ab（可选）
- **版本**: 2.4 或更高
- **用途**: Apache Bench，用于对比测试

### 4.2 监控工具

#### pidstat
- **用途**: 监控进程资源使用
- **安装**: `sudo apt-get install sysstat`

#### perf
- **用途**: 性能分析
- **安装**: `sudo apt-get install linux-tools-common`

#### iostat
- **用途**: 磁盘 I/O 监控
- **安装**: `sudo apt-get install sysstat`

### 4.3 工具验证

在开始测试前，验证所有工具已正确安装：
```bash
wrk --version
ab -V
pidstat -V
perf version
```

## 5. 测试方法标准

### 5.1 编译配置

#### 标准配置（用于基准测试）
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -ENABLE_DEBUG=OFF \
      -ENABLE_COVERAGE=OFF \
      ..
```

#### 调试配置（仅用于开发调试）
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -ENABLE_DEBUG=ON \
      -ENABLE_COVERAGE=ON \
      ..
```

**注意**: 性能基准测试必须使用标准配置。

### 5.2 编译命令
```bash
# 清理旧的构建
make clean

# 编译（使用所有 CPU 核心）
make -j$(nproc)

# 验证编译结果
ls -lh build/dist/lib/libuvhttp.a
```

### 5.3 测试服务器

#### 性能专用测试服务器
- **文件**: `examples/performance_test.c`
- **要求**:
  - 无调试输出（无 printf）
  - 最小化响应体
  - 优化的服务器配置
  - 支持多种测试场景

#### 服务器配置示例
```c
// 推荐配置
config.max_connections = 10000;
config.read_buffer_size = 16384;
config.write_buffer_size = 16384;
config.keepalive_timeout = 60;
config.connection_timeout = 30;
```

#### 启动服务器
```bash
# 设置 CPU 亲和性（可选）
taskset -c 0-7 ./build/dist/bin/performance_test

# 后台运行
nohup ./build/dist/bin/performance_test > /dev/null 2>&1 &
```

### 5.4 测试场景

#### 基础测试场景

##### 场景 1: 低并发测试
```bash
# 参数
线程数: 2
并发连接: 10
测试时长: 10 秒
测试次数: 5 次（取平均值）

# 命令
wrk -t2 -c10 -d10s http://127.0.0.1:8080/
```

##### 场景 2: 中等并发测试
```bash
# 参数
线程数: 4
并发连接: 50
测试时长: 10 秒
测试次数: 5 次（取平均值）

# 命令
wrk -t4 -c50 -d10s http://127.0.0.1:8080/
```

##### 场景 3: 高并发测试
```bash
# 参数
线程数: 8
并发连接: 200
测试时长: 10 秒
测试次数: 5 次（取平均值）

# 命令
wrk -t8 -c200 -d10s http://127.0.0.1:8080/
```

#### 扩展测试场景

##### 场景 4: POST 请求测试
```bash
# 创建 POST 数据文件
echo '{"test":"data"}' > post_data.json

# 运行测试
wrk -t4 -c50 -d10s -s post.lua http://127.0.0.1:8080/api
```

##### 场景 5: 大响应体测试
```bash
# 测试 100KB 响应
wrk -t4 -c50 -d10s http://127.0.0.1:8080/large/100kb

# 测试 1MB 响应
wrk -t4 -c50 -d10s http://127.0.0.1:8080/large/1mb
```

##### 场景 6: TLS 加密测试
```bash
# 测试 HTTPS
wrk -t4 -c50 -d10s https://127.0.0.1:8443/
```

##### 场景 7: WebSocket 测试
```bash
# 使用专门的 WebSocket 测试工具
# 例如: wscat, websocket-bench
```

### 5.5 测试执行流程

#### 标准测试流程
1. **环境准备**
   ```bash
   # 清理系统缓存
   sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches

   # 停止不必要的后台服务
   sudo systemctl stop cron
   ```

2. **启动测试服务器**
   ```bash
   # 确保端口未被占用
   lsof -i :8080

   # 启动服务器
   ./build/dist/bin/performance_test

   # 等待服务器就绪
   sleep 2

   # 验证服务器运行
   curl -I http://127.0.0.1:8080/
   ```

3. **启动资源监控**
   ```bash
   # 监控 CPU 和内存
   pidstat -p <pid> -d -h -r -s -u -w 1 > perf_stats.log &

   # 监控网络
   sar -n DEV 1 > network_stats.log &
   ```

4. **执行测试**
   ```bash
   # 运行测试脚本
   ./test/run_performance_tests.sh
   ```

5. **停止监控**
   ```bash
   # 停止所有监控进程
   pkill pidstat
   pkill sar
   ```

6. **停止服务器**
   ```bash
   pkill performance_test
   ```

7. **收集结果**
   ```bash
   # 整理测试结果
   ./test/collect_results.sh
   ```

### 5.6 重复测试

#### 测试次数要求
- **基础场景**: 每个场景至少运行 5 次
- **扩展场景**: 每个场景至少运行 3 次
- **回归测试**: 每个场景至少运行 10 次

#### 数据处理
```bash
# 计算平均值
awk '{sum+=$1; count++} END {print sum/count}' results.txt

# 计算标准差
awk '{sum+=$1; sumsq+=$1*$1; count++} END {print sqrt(sumsq/count - (sum/count)^2)}' results.txt

# 排除异常值（使用 IQR 方法）
# Q1 = 25th percentile
# Q3 = 75th percentile
# IQR = Q3 - Q1
# 异常值 = < (Q1 - 1.5*IQR) 或 > (Q3 + 1.5*IQR)
```

## 6. 测试数据标准

### 6.1 必需指标

#### 性能指标
- **吞吐量 (QPS)**: 每秒处理的请求数
- **平均延迟**: 所有请求的平均响应时间
- **P50 延迟**: 50% 的请求响应时间
- **P95 延迟**: 95% 的请求响应时间
- **P99 延迟**: 99% 的请求响应时间
- **P99.9 延迟**: 99.9% 的请求响应时间
- **传输速率**: 每秒传输的数据量（MB/s）

#### 资源指标
- **CPU 使用率**: 平均 CPU 使用率
- **内存使用**: 平均内存使用量
- **上下文切换**: 每秒上下文切换次数
- **系统调用**: 每秒系统调用次数

### 6.2 数据记录格式

#### 测试结果记录
```json
{
  "test_id": "PERF-2026-01-09-001",
  "timestamp": "2026-01-09T22:30:00Z",
  "environment": {
    "os": "Linux 6.14.11-2-pve",
    "cpu": "AMD Ryzen 7 5800H",
    "cores": 16,
    "memory": "12GB",
    "compiler": "GCC",
    "compiler_version": "11.4.0"
  },
  "configuration": {
    "build_with_mimalloc": true,
    "build_with_websocket": true,
    "debug": false,
    "optimization": "-O2"
  },
  "test_scenario": {
    "name": "中等并发测试",
    "threads": 4,
    "connections": 50,
    "duration": 10,
    "method": "GET",
    "url": "http://127.0.0.1:8080/"
  },
  "results": {
    "total_requests": 165440,
    "requests_per_second": 16544.0,
    "latency_avg_ms": 2.94,
    "latency_p50_ms": 2.8,
    "latency_p95_ms": 5.5,
    "latency_p99_ms": 8.2,
    "latency_p999_ms": 15.3,
    "transfer_rate_mbps": 5.33
  },
  "resources": {
    "cpu_usage_percent": 85.5,
    "memory_usage_mb": 128.5,
    "context_switches_per_sec": 12500,
    "system_calls_per_sec": 45000
  }
}
```

### 6.3 数据验证

#### 合理性检查
- 吞吐量应在合理范围内（1000-100000 QPS）
- 延迟应随并发增加而增加
- CPU 使用率不应超过 100%
- 内存使用应保持稳定

#### 一致性检查
- 多次测试结果的标准差应小于平均值的 10%
- 不应出现异常值（超出平均值 ±3 个标准差）

## 7. 测试报告标准

### 7.1 报告结构

#### 1. 概述
- 测试目的
- 测试范围
- 测试日期
- 测试人员

#### 2. 测试环境
- 硬件配置
- 软件配置
- 网络配置

#### 3. 测试配置
- 编译配置
- 服务器配置
- 测试工具配置

#### 4. 测试结果
- 基础场景结果
- 扩展场景结果
- 资源使用情况
- 性能指标汇总

#### 5. 分析
- 结果分析
- 性能趋势
- 瓶颈识别
- 优化建议

#### 6. 结论
- 性能评估
- 与基准对比
- 改进建议

### 7.2 报告格式

#### Markdown 格式示例
```markdown
# UVHTTP 性能测试报告

## 测试概述
- **测试日期**: 2026-01-09
- **测试人员**: 张三
- **测试版本**: v1.2.0

## 测试环境
- **操作系统**: Linux 6.14.11-2-pve
- **CPU**: AMD Ryzen 7 5800H (16核)
- **内存**: 12GB

## 测试结果

### 场景 1: 低并发测试
| 指标 | 值 |
|-----|---|
| 吞吐量 | 35,864 QPS |
| 平均延迟 | 271.91 μs |
| P99 延迟 | 1.3 ms |

### 场景 2: 中等并发测试
| 指标 | 值 |
|-----|---|
| 吞吐量 | 16,544 QPS |
| 平均延迟 | 2.94 ms |
| P99 延迟 | 8.2 ms |

## 结论
...
```

### 7.3 报告存储

#### 命名规范
```
PERF-<YYYY-MM-DD>-<VERSION>-<SCENARIO>.md

例如:
PERF-2026-01-09-v1.2.0-baseline.md
PERF-2026-01-09-v1.2.0-regression.md
```

#### 存储位置
```
docs/performance/
├── reports/
│   ├── 2026-01-09/
│   │   ├── PERF-2026-01-09-v1.2.0-baseline.md
│   │   └── PERF-2026-01-09-v1.2.0-regression.md
│   └── 2026-01-10/
│       └── ...
└── data/
    ├── 2026-01-09/
    │   ├── baseline.json
    │   └── regression.json
    └── ...
```

## 8. 性能基准标准

### 8.1 基准性能指标

> **注意**：以下基准值基于实际测试结果得出。

#### 低并发场景（2 线程 / 10 连接）
| 指标 | 最低要求 | 推荐值 | 实际基准 |
|-----|---------|-------|---------|
| 吞吐量 (QPS) | 10,000 | 30,000 | 35,864 |
| 平均延迟 | < 1 ms | < 500 μs | 271.91 μs |
| P99 延迟 | < 10 ms | < 5 ms | 1.3 ms |

#### 中等并发场景（4 线程 / 50 连接）
| 指标 | 最低要求 | 推荐值 | 实际基准 |
|-----|---------|-------|---------|
| 吞吐量 (QPS) | 10,000 | 15,000 | 16,544 |
| 平均延迟 | < 10 ms | < 5 ms | 2.94 ms |
| P99 延迟 | < 50 ms | < 20 ms | 8.2 ms |

#### 高并发场景（8 线程 / 200 连接）
| 指标 | 最低要求 | 推荐值 | 实际基准 |
|-----|---------|-------|---------|
| 吞吐量 (QPS) | 10,000 | 14,000 | 14,015 |
| 平均延迟 | < 50 ms | < 30 ms | 36.63 ms |
| P99 延迟 | < 200 ms | < 100 ms | 121.0 ms |

### 8.2 性能回归阈值

#### 吞吐量回归
- **警告**: 下降 5-10%
- **失败**: 下降 > 10%

#### 延迟回归
- **警告**: 增加 10-20%
- **失败**: 增加 > 20%

#### 资源使用回归
- **警告**: CPU 使用增加 10-20%
- **失败**: CPU 使用增加 > 20%

### 8.3 性能优化目标

> **注意**：以下目标基于实际测试结果制定。

#### 短期目标（1-2 周）
- 低并发吞吐量达到 35,000+ QPS（实际已达到 35,864 QPS）
- 中等并发吞吐量达到 16,000+ QPS（实际已达到 16,544 QPS）
- 高并发吞吐量达到 14,000+ QPS（实际已达到 14,015 QPS）
- 平均延迟降低 20%

#### 中期目标（1-2 月）
- 建立性能回归测试系统
- 集成到 CI/CD 流程
- 定期执行性能测试
- 持续优化性能

#### 长期目标（3-6 月）
- 扩展测试场景
- 支持分布式测试
- 建立性能基准数据库
- 发布性能优化版本

## 9. 质量保证

### 9.1 测试前检查清单

- [ ] 系统配置符合标准
- [ ] 所有工具已安装并验证
- [ ] 使用标准编译配置
- [ ] 编译无警告无错误
- [ ] 测试服务器无调试输出
- [ ] 服务器配置已优化
- [ ] 测试脚本已验证
- [ ] 监控工具已启动

### 9.2 测试中检查清单

- [ ] 服务器正常运行
- [ ] 资源监控正常运行
- [ ] 测试按计划执行
- [ ] 测试结果记录完整
- [ ] 未发生异常中断

### 9.3 测试后检查清单

- [ ] 所有测试场景已完成
- [ ] 测试数据已收集
- [ ] 数据已验证合理性
- [ ] 异常值已处理
- [ ] 测试报告已生成
- [ ] 报告已存储

## 10. 变更管理

### 10.1 标准变更流程

1. 提出变更申请
2. 评估变更影响
3. 获得批准
4. 更新文档
5. 通知相关人员
6. 培训相关人员

### 10.2 版本历史

| 版本 | 日期 | 变更内容 | 作者 |
|-----|------|---------|------|
| 1.0.0 | 2026-01-09 | 初始版本 | UVHTTP 开发团队 |

## 11. 附录

### 11.1 术语表

- **QPS**: Requests Per Second，每秒请求数
- **P50/P95/P99**: 百分位数，表示 50%/95%/99% 的请求响应时间
- **IQR**: Interquartile Range，四分位距
- **mimalloc**: Microsoft 的内存分配器，性能优于系统分配器

### 11.2 参考资料

- [wrk 官方文档](https://github.com/wg/wrk)
- [Apache Bench 文档](https://httpd.apache.org/docs/2.4/programs/ab.html)
- [Linux 性能调优指南](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/)

### 11.3 联系方式

---

## 附录 A: 性能测试计划

> **注意**：本附录包含性能测试计划的详细内容。如需了解测试执行流程、测试脚本和风险管理，请参考完整内容。

### A.1 测试目标

本测试计划旨在全面评估 UVHTTP 项目在不同场景下的性能表现，包括：

- 建立性能基准数据
- 识别性能瓶颈
- 验证优化效果
- 确保性能稳定性
- 支持性能回归测试

### A.2 测试范围

#### 功能范围
- ✅ HTTP/1.1 请求处理
- ✅ Keep-Alive 连接管理
- ✅ 路由功能
- ✅ 响应处理
- ✅ 中间件系统
- ✅ TLS/HTTPS 加密通信
- ✅ WebSocket 通信
- ✅ 静态文件服务
- ✅ 限流功能
- ✅ CORS 支持

#### 场景范围
- ✅ 不同并发级别（低、中、高）
- ✅ 不同请求方法（GET、POST、PUT、DELETE）
- ✅ 不同响应体大小
- ✅ 长时间运行稳定性
- ✅ 内存泄漏检测

### A.3 测试阶段

#### 第一阶段：环境准备
- 验证测试环境配置
- 安装和验证测试工具
- 编译 UVHTTP 项目（标准配置）
- 创建性能专用测试服务器
- 编写测试脚本
- 设置监控系统

#### 第二阶段：基础性能测试
- 低并发测试（2 线程 / 10 连接）
- 中等并发测试（4 线程 / 50 连接）
- 高并发测试（8 线程 / 200 连接）
- 不同线程数测试（1、2、4、8、16）
- 数据分析和报告

#### 第三阶段：扩展性能测试
- POST 请求测试
- 大响应体测试（1KB、10KB、100KB、1MB）
- TLS/HTTPS 加密测试
- WebSocket 性能测试
- 静态文件服务测试
- 数据分析和报告

#### 第四阶段：稳定性测试
- 长时间运行测试（30 分钟）
- 长时间运行测试（1 小时）
- 长时间运行测试（4 小时）
- 内存泄漏检测
- 数据分析和报告

#### 第五阶段：优化和回归测试
- 识别性能瓶颈
- 实施性能优化
- 重新测试验证优化效果
- 建立性能基准数据库
- 编写性能测试指南
- 生成最终报告

### A.4 测试脚本示例

#### 主测试脚本
```bash
#!/bin/bash
# test/run_performance_tests.sh

set -e

# 配置
TEST_DIR="test/performance_results"
SERVER_PID=""
MONITOR_PID=""

# 创建结果目录
mkdir -p $TEST_DIR

# 启动服务器
start_server() {
    echo "启动性能测试服务器..."
    ./build/dist/bin/performance_test &
    SERVER_PID=$!
    sleep 2

    # 验证服务器运行
    if ! curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/ | grep -q "200"; then
        echo "错误: 服务器启动失败"
        exit 1
    fi
    echo "服务器已启动 (PID: $SERVER_PID)"
}

# 运行测试场景
run_test() {
    local name=$1
    local threads=$2
    local connections=$3
    local duration=$4
    local iterations=$5

    echo "运行测试场景: $name"
    echo "参数: 线程=$threads, 连接=$connections, 时长=${duration}s"

    for i in $(seq 1 $iterations); do
        echo "  迭代 $i/$iterations..."
        wrk -t$threads -c$connections -d${duration}s \
            http://127.0.0.1:8080/ \
            | tee $TEST_DIR/${name}_${i}.txt
    done
}

# 清理函数
cleanup() {
    echo "清理资源..."
    stop_monitor
    stop_server
    exit 0
}

# 捕获退出信号
trap cleanup EXIT INT TERM

# 主流程
main() {
    echo "=== UVHTTP 性能测试 ==="
    echo "开始时间: $(date)"
    echo ""

    # 启动服务器
    start_server

    # 运行测试场景
    echo "=== 基础性能测试 ==="
    run_test "low_concurrent" 2 10 10 5
    run_test "medium_concurrent" 4 50 10 5
    run_test "high_concurrent" 8 200 10 5

    echo ""
    echo "=== 测试完成 ==="
    echo "结束时间: $(date)"
    echo "结果保存在: $TEST_DIR"
}

# 执行主流程
main
```

### A.5 风险管理

#### 潜在风险

**风险 1: 测试环境不稳定**
- **影响**: 测试结果不可靠
- **概率**: 中
- **缓解措施**:
  - 使用专用测试服务器
  - 关闭不必要的后台服务
  - 固定 CPU 频率

**风险 2: 测试工具问题**
- **影响**: 无法完成测试
- **概率**: 低
- **缓解措施**:
  - 提前验证测试工具
  - 准备备用测试工具
  - 记录详细的错误日志

### A.6 交付物

#### 测试数据
- 原始测试结果
- 资源监控数据
- 系统日志

#### 测试报告
- 性能测试报告
- 性能分析报告
- 优化建议报告

#### 测试工具
- 测试脚本
- 分析脚本
- 监控脚本

---

**文档结束**

- **维护者**: UVHTTP 开发团队
- **邮箱**: dev@uvhttp.org
- **Issue**: https://github.com/adam-ikari/uvhttp/issues

---

**文档结束**