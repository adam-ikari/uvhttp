# UVHTTP CI/CD 方案设计文档

## 文档信息

- **项目**: UVHTTP
- **版本**: 2.2.0
- **创建日期**: 2026-02-23
- **文档类型**: 技术方案设计
- **状态**: 已实现

---

## 目录

1. [概述](#概述)
2. [设计原则](#设计原则)
3. [工作流架构](#工作流架构)
4. [核心工作流](#核心工作流)
5. [可复用 Actions](#可复用-actions)
6. [配置矩阵](#配置矩阵)
7. [触发条件](#触发条件)
8. [并发控制](#并发控制)
9. [构建配置](#构建配置)
10. [测试策略](#测试策略)
11. [覆盖率管理](#覆盖率管理)
12. [性能监控](#性能监控)
13. [安全扫描](#安全扫描)
14. [发布流程](#发布流程)
15. [监控与告警](#监控与告警)
16. [最佳实践](#最佳实践)
17. [附录](#附录)

---

## 概述

### 背景

UVHTTP 是一个基于 libuv 的高性能 HTTP/1.1 和 WebSocket 服务器库，采用 C11 标准编写。为了确保代码质量、性能稳定性和持续交付能力，需要构建一套完善的 CI/CD 系统。

### 目标

1. **代码质量保障**: 通过自动化测试、静态分析确保代码质量
2. **性能监控**: 持续监控性能指标，及时发现性能退化
3. **安全扫描**: 定期进行安全漏洞扫描和依赖检查
4. **快速反馈**: 在 PR 阶段提供快速反馈，加速开发迭代
5. **自动化发布**: 实现从代码到发布的全自动化流程
6. **多平台支持**: 支持 Linux 32/64 位、macOS、Windows 等平台
7. **配置验证**: 验证各种编译配置的正确性

### 技术栈

- **CI/CD 平台**: GitHub Actions
- **构建系统**: CMake 3.10+
- **测试框架**: Google Test
- **性能测试**: wrk、ab
- **代码覆盖率**: lcov、gcov
- **静态分析**: cppcheck、clang-tidy
- **安全扫描**: CodeQL、Dependency Check
- **缓存**: GitHub Actions Cache

---

## 设计原则

### 1. 快速反馈 (Fast Feedback)

- **PR 阶段**: 仅运行必要的测试，在 10-15 分钟内完成
- **矩阵验证**: 使用并行策略，同时验证多个配置
- **增量测试**: 优先测试变更相关的模块

### 2. 全面覆盖 (Comprehensive Coverage)

- **构建矩阵**: 覆盖 14 种编译配置组合
- **平台覆盖**: Linux (32/64 位)、macOS、Windows
- **测试类型**: 单元测试、集成测试、性能测试、压力测试、内存测试

### 3. 分级验证 (Tiered Validation)

```
Level 1: PR 快速验证 (10-15 分钟)
  ├─ 构建
  ├─ 快速测试
  ├─ 代码格式检查
  └─ 依赖扫描

Level 2: Push 全面验证 (20-30 分钟)
  ├─ 构建矩阵 (14 配置)
  ├─ 完整测试
  ├─ 静态分析
  └─ 性能基准测试

Level 3: 每日深度测试 (60-90 分钟)
  ├─ 覆盖率报告
  ├─ 内存泄漏检测
  ├─ 压力测试
  ├─ 性能趋势分析
  └─ 安全扫描

Level 4: 发布验证 (30-40 分钟)
  ├─ 多平台构建
  ├─ 完整测试套件
  ├─ 发布包打包
  └─ Release 创建
```

### 4. 可维护性 (Maintainability)

- **可复用 Actions**: 将通用逻辑封装为可复用 Actions
- **配置驱动**: 使用矩阵配置，避免重复代码
- **文档完善**: 每个工作流都有详细的注释和文档

### 5. 成本优化 (Cost Optimization)

- **智能缓存**: 缓存依赖和构建产物，减少重复编译
- **并行执行**: 利用矩阵策略并行执行任务
- **按需触发**: 根据事件类型选择合适的工作流

---

## 工作流架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                     GitHub Actions 触发                      │
│  PR | Push | Tag | Schedule | Manual Dispatch                │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │   PR    │   │  Push   │   │  Nightly │
   │  验证   │   │  验证   │   │  测试    │
   └────┬────┘   └────┬────┘   └────┬────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
              ┌────────▼────────┐
              │   共享 Actions  │
              │  setup-build    │
              │  cache-deps     │
              │  run-tests      │
              └────────┬────────┘
                       │
              ┌────────▼────────┐
              │  构建矩阵验证   │
              │  (14 配置)      │
              └────────┬────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │  测试   │   │  分析   │   │  性能   │
   │  执行   │   │  报告   │   │  监控   │
   └────┬────┘   └────┬────┘   └────┬────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
              ┌────────▼────────┐
              │   结果汇总      │
              │   GitHub Summary│
              └─────────────────┘
```

### 工作流列表

| 工作流名称 | 触发条件 | 用途 | 执行时间 |
|-----------|---------|------|---------|
| `ci-pr.yml` | PR 到 main/develop | PR 快速验证 | 10-15 分钟 |
| `ci-build-matrix.yml` | Push 到 main/develop/pre-release | 构建矩阵验证 | 20-30 分钟 |
| `ci-32bit.yml` | Push 到 main/develop/feature/* | 32 位编译验证 | 2-3 分钟 |
| `ci-performance.yml` | Push 到 develop | 性能基准测试 | 15-20 分钟 |
| `ci-performance-tls.yml` | Push 到 develop | TLS 性能测试 | 15-20 分钟 |
| `ci-nightly.yml` | 每日 UTC 0:00 | 深度测试和报告 | 60-90 分钟 |
| `ci-release.yml` | Push tag (v*) | 发布流程 | 30-40 分钟 |
| `deploy-docs.yml` | Push to main | 文档部署 | 5-10 分钟 |
| `performance-benchmark.yml` | Manual | 性能基准对比 | 20-30 分钟 |
| `security-issue-creator.yml` | Security alerts | 自动创建安全 Issue | 即时 |
| `notify.yml` | 工作流完成 | 通知和报告 | 即时 |

---

## 核心工作流

### 1. PR 快速验证 (ci-pr.yml)

#### 目标

在 PR 提交时提供快速反馈，确保代码可以编译并通过基本测试。

#### 触发条件

```yaml
on:
  pull_request:
    branches: [ main, develop ]
    types: [opened, synchronize, reopened]
```

#### 任务列表

```
阶段 1: 并行执行
├─ ubuntu-build: Ubuntu 构建和测试
├─ code-quality-check: 代码质量检查
├─ dependency-scan: 依赖漏洞扫描
├─ ubuntu-test-fast: 快速测试
└─ format-check: 代码格式检查

阶段 2: 结果汇总
└─ generate-summary: 生成 PR 摘要
```

#### 执行时间

- **总时间**: 10-15 分钟
- **并行任务**: 5 个
- **关键路径**: ubuntu-build → ubuntu-test-fast → generate-summary

#### 验证内容

| 类别 | 验证项 | 工具 |
|-----|-------|------|
| 构建 | 64 位 Release 构建 | CMake |
| 测试 | 快速测试集 | Google Test |
| 质量 | cppcheck 静态分析 | cppcheck |
| 质量 | 代码格式检查 | clang-format |
| 安全 | 依赖漏洞扫描 | npm audit |

#### 输出产物

- 构建产物: `build-ubuntu-pr-{PR_NUMBER}.zip`
- 测试日志: `test-logs-ubuntu-pr-{PR_NUMBER}.zip`
- 质量报告: `code-quality-pr-{PR_NUMBER}.zip`
- GitHub Summary: PR 页面显示测试结果

#### 失败策略

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.event.pull_request.number }}
  cancel-in-progress: true
```

- 同一 PR 的新提交会取消旧的运行
- 任何任务失败都会导致整体失败
- 失败任务会保留日志供调试

---

### 2. 构建矩阵验证 (ci-build-matrix.yml)

#### 目标

验证所有编译配置组合的正确性，确保库在各种配置下都能正常工作。

#### 触发条件

```yaml
on:
  pull_request:
    branches: [ main, develop ]
    types: [opened, synchronize, reopened]
  push:
    branches: [ main, develop, pre-release ]
  workflow_dispatch:
```

#### 配置矩阵

```yaml
strategy:
  fail-fast: false
  matrix:
    config:
      # 基础配置
      - name: "Minimal (no optional features)"
        websocket: OFF
        mimalloc: OFF
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      # 全功能配置
      - name: "Full Features"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      # 单独功能测试
      - name: "WebSocket Only"
        websocket: ON
        mimalloc: OFF
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "HTTPS Only"
        websocket: OFF
        mimalloc: OFF
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "mimalloc Only"
        websocket: OFF
        mimalloc: ON
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      # 特殊模式
      - name: "Debug Mode"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: ON
        coverage: OFF
        examples: OFF

      - name: "Coverage Mode"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: ON
        examples: OFF

      # 组合测试
      - name: "WebSocket + HTTPS"
        websocket: ON
        mimalloc: OFF
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "WebSocket + mimalloc"
        websocket: ON
        mimalloc: ON
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "HTTPS + mimalloc"
        websocket: OFF
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "WebSocket + HTTPS + mimalloc"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "Debug + Coverage"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: ON
        coverage: ON
        examples: OFF

      - name: "Minimal + Debug"
        websocket: OFF
        mimalloc: OFF
        https: OFF
        debug: ON
        coverage: OFF
        examples: OFF

      - name: "Full + Examples"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: ON
```

#### 配置说明

| 配置 | WebSocket | mimalloc | HTTPS | Debug | Coverage | Examples |
|-----|-----------|----------|-------|-------|----------|----------|
| Minimal | OFF | OFF | OFF | OFF | OFF | OFF |
| Full Features | ON | ON | ON | OFF | OFF | OFF |
| WebSocket Only | ON | OFF | OFF | OFF | OFF | OFF |
| HTTPS Only | OFF | OFF | ON | OFF | OFF | OFF |
| mimalloc Only | OFF | ON | OFF | OFF | OFF | OFF |
| Debug Mode | ON | ON | ON | ON | OFF | OFF |
| Coverage Mode | ON | ON | ON | OFF | ON | OFF |
| WebSocket + HTTPS | ON | OFF | ON | OFF | OFF | OFF |
| WebSocket + mimalloc | ON | ON | OFF | OFF | OFF | OFF |
| HTTPS + mimalloc | OFF | ON | ON | OFF | OFF | OFF |
| WebSocket + HTTPS + mimalloc | ON | ON | ON | OFF | OFF | OFF |
| Debug + Coverage | ON | ON | ON | ON | ON | OFF |
| Minimal + Debug | OFF | OFF | OFF | ON | OFF | OFF |
| Full + Examples | ON | ON | ON | OFF | OFF | ON |

#### 执行时间

- **单配置**: 2-3 分钟
- **总时间**: 20-30 分钟 (并行执行 14 个配置)

#### 验证内容

| 类别 | 验证项 | 标准 |
|-----|-------|------|
| 构建 | CMake 配置成功 | 退出码 0 |
| 构建 | 编译无警告 | 无 "warning:" 输出 |
| 测试 | 测试通过 | 所有测试通过 |
| 测试 | 测试跳过处理 | 退出码 8 表示预期跳过 |
| 覆盖率 | 覆盖率报告生成 | coverage.info 存在 |
| 产物 | 构建产物完整 | libuvhttp.a 存在 |

#### 输出产物

- 每个配置的构建产物: `build-{CONFIG_NAME}-{PR_NUMBER}.zip`
- 测试日志: `test-logs-{CONFIG_NAME}-{PR_NUMBER}.zip`
- 覆盖率报告: 仅 Coverage 配置生成

#### 失败策略

```yaml
strategy:
  fail-fast: false
```

- `fail-fast: false`: 即使某个配置失败，其他配置继续执行
- 便于一次性发现所有配置问题
- 每个配置独立运行，互不影响

---

### 3. 32 位编译验证 (ci-32bit.yml)

#### 目标

验证 32 位 (i686) 架构下的编译正确性，确保库可以用于 32 位系统。

#### 触发条件

```yaml
on:
  push:
    branches: [ main, develop, 'feature/**' ]
  pull_request:
    branches: [ main, develop ]
  workflow_dispatch:
```

#### 技术挑战

1. **交叉编译**: 在 64 位系统上编译 32 位代码
2. **依赖库**: 所有依赖库 (libuv, mbedtls, llhttp) 必须编译为 32 位
3. **C++ 链接**: 32 位 libstdc++ 在 CI/CD 环境中的链接问题
4. **架构验证**: 需要验证生成的库确实是 32 位

#### 解决方案

##### 1. 32 位工具链

```yaml
- name: Install 32-bit toolchain
  run: |
    sudo apt-get update
    sudo apt-get install -y gcc-multilib g++-multilib g++-12-multilib
```

##### 2. 编译器标志

```yaml
cmake -B build-32bit \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DEXE_LINKER_FLAGS="-m32" \
  -DSHARED_LINKER_FLAGS="-m32" \
  -DMODULE_LINKER_FLAGS="-m32" \
  -DBUILD_WITH_WEBSOCKET=ON \
  -DBUILD_WITH_MIMALLOC=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DENABLE_COVERAGE=OFF
```

关键标志说明:

| 标志 | 作用 |
|-----|------|
| `-m32` | 生成 32 位代码 |
| `-march=i686` | 指定目标架构 |
| `-D_GNU_SOURCE` | 启用 GNU 扩展 |
| `-Wno-format-truncation` | 禁用格式截断警告 |

##### 3. 依赖库编译

修改 `cmake/Dependencies.cmake`，将编译器标志传递给依赖库:

```cmake
execute_process(
  COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv -B ${LIBUV_BUILD_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_TESTING=OFF
    -DLIBUV_BUILD_SHARED=OFF
    -DLIBUV_BUILD_BENCH=OFF
    -DLIBUV_BUILD_EXAMPLES=OFF
    "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
  RESULT_VARIABLE LIBUV_CONFIG_RESULT
)
```

##### 4. 跳过 C++ 测试

由于 32 位 libstdc++ 链接问题，仅构建核心库，不编译测试:

```yaml
cmake --build build-32bit --config Release --target uvhttp -j$(nproc)
```

##### 5. 架构验证

使用 `objdump` 验证库文件为 32 位:

```bash
objdump -f build-32bit/dist/lib/libuvhttp.a | grep -q "elf32-i386"
```

#### 任务列表

```
阶段 1: 构建
└─ ubuntu-32bit-build: 32 位编译和验证

阶段 2: 测试
└─ ubuntu-32bit-test: 基础验证

阶段 3: 总结
└─ generate-summary: 生成摘要
```

#### 执行时间

- **总时间**: 2-3 分钟
- **构建**: 1.5 分钟
- **测试**: 0.5 分钟
- **验证**: 即时

#### 验证内容

| 类别 | 验证项 | 工具/方法 |
|-----|-------|----------|
| 构建 | 32 位编译成功 | CMake + gcc -m32 |
| 架构 | 主库为 32 位 | objdump -f |
| 架构 | libuv 为 32 位 | objdump -f |
| 架构 | mbedtls 为 32 位 | objdump -f |
| 架构 | llhttp 为 32 位 | objdump -f |
| 文件 | 库文件存在 | file + stat |
| 文件 | 库文件非空 | file size > 0 |

#### 输出产物

- 构建产物: `build-32bit-{SHA}.zip`
- 测试日志: `test-logs-32bit-{SHA}.zip`
- GitHub Summary: 32 位验证摘要

#### 限制和说明

1. **C++ 测试跳过**: 由于 CI/CD 环境的 32 位 libstdc++ 链接问题，不编译 C++ 测试
2. **mimalloc 禁用**: 32 位环境不使用 mimalloc 分配器
3. **WebSocket 启用**: 支持 WebSocket 功能的 32 位编译

#### 本地验证

开发者可以在本地验证 32 位编译:

```bash
# 安装 32 位工具链
sudo apt-get install gcc-multilib g++-multilib

# 配置并编译 32 位版本
cmake -B build-32bit \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"

cmake --build build-32bit --config Release

# 验证架构
file build-32bit/dist/lib/libuvhttp.a
objdump -f build-32bit/dist/lib/libuvhttp.a | head -1
```

---

### 4. 性能基准测试 (ci-performance.yml)

#### 目标

监控性能指标，及时发现性能退化。

#### 触发条件

```yaml
on:
  push:
    branches: [ develop ]
  workflow_dispatch:
```

#### 测试场景

| 场景 | 并发数 | 持续时间 | 目标 |
|-----|-------|---------|------|
| 低并发 | 10 | 30s | 基准性能 |
| 中并发 | 50 | 30s | 常规负载 |
| 高并发 | 100 | 30s | 峰值性能 |
| 极限并发 | 500 | 30s | 压力测试 |
| 持续负载 | 100 | 60s | 稳定性 |

#### 测试工具

```bash
# 安装 wrk
sudo apt-get install wrk

# 运行测试
wrk -t4 -c100 -d30s http://localhost:18081/
```

#### 性能指标

| 指标 | 目标值 | 说明 |
|-----|-------|------|
| RPS (低并发) | > 30,000 | 10 连接 |
| RPS (中并发) | > 30,000 | 50 连接 |
| RPS (高并发) | > 30,000 | 100 连接 |
| RPS (极限并发) | > 25,000 | 500 连接 |
| 错误率 | < 0.1% | 所有场景 |
| P50 延迟 | < 5ms | 中位数 |
| P95 延迟 | < 20ms | 95% 请求 |
| P99 延迟 | < 50ms | 99% 请求 |

#### 执行流程

```
1. 启动测试服务器
   ├─ 编译 benchmark_unified
   ├─ 启动服务器 (端口 18081)
   └─ 等待服务器就绪 (3s)

2. 运行性能测试
   ├─ 测试 1: 低并发 (10 connections, 30s)
   ├─ 测试 2: 中并发 (50 connections, 30s)
   ├─ 测试 3: 高并发 (100 connections, 30s)
   ├─ 测试 4: 极限并发 (500 connections, 30s)
   └─ 测试 5: 持续负载 (100 connections, 60s)

3. 解析测试结果
   ├─ 提取 RPS 指标
   ├─ 提取延迟指标
   └─ 提取错误率

4. 生成报告
   ├─ JSON 格式结果
   ├─ Markdown 报告
   └─ 性能趋势图

5. 对比基线
   ├─ 读取基线数据
   ├─ 计算性能变化
   └─ 标记退化 (>5%)
```

#### 输出产物

- 性能结果: `performance-results.json`
- 测试日志: `performance.log`
- 性能报告: `performance-report.md`
- 趋势图: `performance-trend.png`

#### 性能退化检测

```python
# 对比基线
baseline_rps = 23000
current_rps = 21000
degradation = (baseline_rps - current_rps) / baseline_rps * 100

if degradation > 5:
    print(f"⚠️ 性能退化: {degradation:.2f}%")
```

#### 基线管理

- **基线位置**: `config/performance-baseline.yml`
- **更新时机**: 性能优化后、架构变更后
- **验证流程**: 运行 3 次，取平均值

#### 执行时间

- **总时间**: 15-20 分钟
- **服务器启动**: 1 分钟
- **测试执行**: 10-15 分钟
- **报告生成**: 2 分钟

---

### 5. TLS 性能测试 (ci-performance-tls.yml)

#### 目标

验证 TLS 加密对性能的影响，确保 TLS 性能满足要求。

#### 触发条件

```yaml
on:
  push:
    branches: [ develop ]
  workflow_dispatch:
```

#### 测试场景

| 场景 | 加密方式 | 并发数 | 持续时间 |
|-----|---------|-------|---------|
| HTTPS (TLS 1.2) | AES-256-GCM | 100 | 30s |
| HTTPS (TLS 1.3) | AES-256-GCM | 100 | 30s |
| HTTPS (TLS 1.3) | ChaCha20-Poly1305 | 100 | 30s |

#### 性能指标

| 指标 | HTTPS 目标 | HTTP 目标 | 下降限制 |
|-----|-----------|----------|---------|
| RPS | > 20,000 | > 30,000 | < 35% |
| P50 延迟 | < 10ms | < 5ms | < 2x |
| P95 延迟 | < 40ms | < 20ms | < 2x |

#### 输出产物

- HTTPS 性能结果: `https-performance-results.json`
- HTTPS 性能报告: `https-performance-report.md`
- HTTP vs HTTPS 对比: `https-comparison.md`

---

### 6. 每日深度测试 (ci-nightly.yml)

#### 目标

每日运行深度测试，生成覆盖率报告、内存泄漏检测、压力测试和性能趋势分析。

#### 触发条件

```yaml
on:
  schedule:
    - cron: '0 0 * * *'  # 每日 UTC 0:00
  workflow_dispatch:
```

#### 任务列表

```
阶段 1: 并行构建和扫描
├─ ubuntu-build-all: Debug + Coverage 构建
├─ code-quality-full: 完整代码质量检查
└─ security-scan-full: 完整安全扫描

阶段 2: 完整测试
├─ ubuntu-test-full: 完整测试套件
└─ test-coverage: 覆盖率报告生成

阶段 3: 深度测试
├─ coverage-report: 覆盖率 HTML 报告
├─ test-stress: 压力测试 (5 分钟)
├─ performance-full: 完整性能基准
└─ test-memory: 内存泄漏检测 (ASan)

阶段 4: 生成总结
└─ generate-summary: 生成每日报告
```

#### 执行时间

- **总时间**: 60-90 分钟
- **构建**: 25 分钟
- **测试**: 15 分钟
- **覆盖率**: 20 分钟
- **压力测试**: 35 分钟
- **性能测试**: 90 分钟
- **内存测试**: 25 分钟

#### 测试详情

##### 1. 覆盖率报告

```bash
# 生成覆盖率数据
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/deps/*' --output-file coverage.info
lcov --remove coverage.info '*/test/*' --output-file coverage.info

# 生成 HTML 报告
genhtml coverage.info --output-directory coverage-report
```

覆盖率目标:

| 模块 | 目标覆盖率 | 当前覆盖率 |
|-----|-----------|-----------|
| 整体 | 80% | 42.9% |
| uvhttp_static.c | 80% | 17.2% |
| uvhttp_router.c | 80% | 32.3% |
| uvhttp_connection.c | 80% | 32.2% |

##### 2. 压力测试

```bash
# 持续高负载测试 (5 分钟)
wrk -t8 -c500 -d300s --timeout 10s http://localhost:18081/
```

验证内容:

- 错误率 < 0.1%
- 无内存泄漏
- 无崩溃
- 响应时间稳定

##### 3. 内存泄漏检测

```bash
# 使用 AddressSanitizer 编译
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

# 运行测试
ctest --output-on-failure
```

验证内容:

- 无内存泄漏
- 无使用后释放
- 无双重释放
- 无缓冲区溢出

##### 4. 性能趋势分析

运行 8 种性能测试场景:

1. 低并发 (10 connections, 30s)
2. 中并发 (50 connections, 30s)
3. 高并发 (100 connections, 30s)
4. 极限并发 (500 connections, 30s)
5. 超高并发 (1000 connections, 30s)
6. 持续负载 (100 connections, 60s)
7. 小文件传输
8. 大文件传输

生成性能趋势图，对比历史数据。

#### 输出产物

- 覆盖率报告: `coverage-report-nightly-{RUN_NUMBER}.zip`
- 性能结果: `performance-full-nightly-{RUN_NUMBER}.zip`
- 压力测试结果: `stress-test-results-nightly-{RUN_NUMBER}.zip`
- 内存测试结果: `memory-test-results-nightly-{RUN_NUMBER}.zip`
- 安全扫描结果: `security-results-nightly-{RUN_NUMBER}.zip`
- 代码质量结果: `code-quality-results-nightly-{RUN_NUMBER}.zip`

#### Nightly Release

创建一个预发布的 nightly 版本，包含所有测试产物:

```yaml
- name: Create nightly release
  uses: softprops/action-gh-release@v1
  with:
    tag_name: nightly-${{ github.run_number }}
    name: Nightly Build - ${{ github.event.head_commit.timestamp }}
    body_path: performance-trend.md
    draft: false
    prerelease: true
```

---

### 7. 发布流程 (ci-release.yml)

#### 目标

自动化发布流程，从代码到 Release 的全自动化。

#### 触发条件

```yaml
on:
  push:
    tags:
      - 'v*'  # 如 v1.5.0, v2.0.0
  workflow_dispatch:
```

#### 任务列表

```
阶段 1: Linux Release 构建
└─ ubuntu-release-build: Release 模式构建

阶段 2: 发布测试
└─ release-test: 完整测试套件

阶段 3: 创建 Release
└─ create-release: 创建 GitHub Release
```

#### 执行时间

- **总时间**: 30-40 分钟
- **构建**: 15 分钟
- **测试**: 20 分钟
- **打包**: 2 分钟
- **创建 Release**: 3 分钟

#### 发布流程

```
1. 解析版本号
   ├─ 从 tag 提取版本号 (如 v2.3.0 → 2.3.0)
   └─ 验证版本号格式

2. 构建发布版本
   ├─ CMake Release 模式
   ├─ 编译所有模块
   └─ 运行完整测试

3. 打包发布产物
   ├─ 复制库文件 (libuvhttp.a)
   ├─ 复制头文件 (include/)
   ├─ 创建归档 (tar.gz)
   └─ 计算校验和 (SHA256)

4. 创建 GitHub Release
   ├─ 使用 softprops/action-gh-release
   ├─ 上传归档文件
   ├─ 自动生成 Release Notes
   └─ 标记为正式发布
```

#### 发布产物

- Linux x86_64: `uvhttp-{VERSION}-linux-x86_64.tar.gz`
- 校验和: `uvhttp-{VERSION}-SHA256.txt`
- Release Notes: 自动生成

#### 版本号规范

遵循语义化版本 (SemVer):

```
MAJOR.MINOR.PATCH

MAJOR: 不兼容的 API 变更
MINOR: 向后兼容的功能新增
PATCH: 向后兼容的问题修复
```

示例:

- `v2.3.0`: 2.3.0 正式发布
- `v2.3.1`: 2.3.0 的补丁版本
- `v3.0.0`: 3.0.0 重大版本升级

#### Release 检查清单

发布前必须确保:

- [ ] 所有 CI/CD 测试通过
- [ ] 代码覆盖率达标 (80%+)
- [ ] 性能测试通过
- [ ] 安全扫描无高危漏洞
- [ ] 文档已更新
- [ ] CHANGELOG 已更新
- [ ] 版本号已更新
- [ ] Git 标签已创建

---

## 可复用 Actions

### 1. setup-build

#### 路径

`.github/actions/setup-build`

#### 功能

设置构建环境，包括:

- 安装 CMake
- 安装编译器
- 配置编译选项

#### 使用示例

```yaml
- name: Setup build environment
  uses: ./.github/actions/setup-build
  with:
    os: ubuntu-latest
```

#### 参数

| 参数 | 类型 | 必填 | 说明 |
|-----|------|------|------|
| os | string | 是 | 操作系统 (ubuntu-latest, macos-latest, windows-latest) |

---

### 2. cache-deps

#### 路径

`.github/actions/cache-deps`

#### 功能

缓存依赖库，减少重复编译时间。

#### 使用示例

```yaml
- name: Cache dependencies
  uses: ./.github/actions/cache-deps
  with:
    cache-key: pr-${{ github.event.pull_request.number }}
    build-type: Release
```

#### 参数

| 参数 | 类型 | 必填 | 说明 |
|-----|------|------|------|
| cache-key | string | 是 | 缓存键名 |
| build-type | string | 是 | 构建类型 (Debug, Release) |

#### 缓存策略

```yaml
- uses: actions/cache@v3
  with:
    path: |
      deps/*/build
      build/CMakeCache.txt
      build/CMakeFiles
    key: ${{ inputs.cache-key }}-${{ runner.os }}-${{ inputs.build-type }}-${{ hashFiles('CMakeLists.txt', 'cmake/*.cmake') }}
    restore-keys: |
      ${{ inputs.cache-key }}-${{ runner.os }}-${{ inputs.build-type }}-
      ${{ inputs.cache-key }}-${{ runner.os }}-
```

---

### 3. run-tests

#### 路径

`.github/actions/run-tests`

#### 功能

运行测试套件，支持不同类型的测试。

#### 使用示例

```yaml
- name: Run fast tests
  uses: ./.github/actions/run-tests
  with:
    build-dir: build
    test-type: fast
    timeout: 60
    parallel: $(nproc)
```

#### 参数

| 参数 | 类型 | 必填 | 说明 |
|-----|------|------|------|
| build-dir | string | 是 | 构建目录 |
| test-type | string | 是 | 测试类型 (fast, all, coverage, memory) |
| timeout | number | 否 | 超时时间 (秒)，默认 60 |
| parallel | number | 否 | 并行数，默认 1 |

#### 测试类型

| 类型 | 说明 | 用途 |
|-----|------|------|
| fast | 快速测试集 | PR 验证 |
| all | 完整测试套件 | Push 验证 |
| coverage | 覆盖率测试 | Nightly 测试 |
| memory | 内存测试 (ASan) | Nightly 测试 |

---

## 配置矩阵

### 构建矩阵总结

| 配置 | WebSocket | mimalloc | HTTPS | Debug | Coverage | Examples | 用途 |
|-----|-----------|----------|-------|-------|----------|----------|------|
| Minimal | OFF | OFF | OFF | OFF | OFF | OFF | 最小依赖验证 |
| Full Features | ON | ON | ON | OFF | OFF | OFF | 全功能验证 |
| WebSocket Only | ON | OFF | OFF | OFF | OFF | OFF | WebSocket 独立测试 |
| HTTPS Only | OFF | OFF | ON | OFF | OFF | OFF | HTTPS 独立测试 |
| mimalloc Only | OFF | ON | OFF | OFF | OFF | OFF | mimalloc 独立测试 |
| Debug Mode | ON | ON | ON | ON | OFF | OFF | Debug 模式验证 |
| Coverage Mode | ON | ON | ON | OFF | ON | OFF | 覆盖率测试 |
| WebSocket + HTTPS | ON | OFF | ON | OFF | OFF | OFF | 组合验证 |
| WebSocket + mimalloc | ON | ON | OFF | OFF | OFF | OFF | 组合验证 |
| HTTPS + mimalloc | OFF | ON | ON | OFF | OFF | OFF | 组合验证 |
| WebSocket + HTTPS + mimalloc | ON | ON | ON | OFF | OFF | OFF | 完整组合 |
| Debug + Coverage | ON | ON | ON | ON | ON | OFF | 调试 + 覆盖率 |
| Minimal + Debug | OFF | OFF | OFF | ON | OFF | OFF | 最小 + Debug |
| Full + Examples | ON | ON | ON | OFF | OFF | ON | 完整 + 示例 |

### 矩阵验证策略

1. **独立验证**: 每个配置独立运行，互不影响
2. **失败不停止**: `fail-fast: false`，发现所有问题
3. **并行执行**: 利用 GitHub Actions 并行能力
4. **结果汇总**: 统一生成测试摘要

---

## 触发条件

### 触发类型

| 触发类型 | 触发条件 | 工作流 |
|---------|---------|-------|
| PR | PR 到 main/develop | ci-pr.yml |
| Push | Push 到 main/develop/pre-release | ci-build-matrix.yml |
| Push | Push 到 main/develop/feature/* | ci-32bit.yml |
| Push | Push 到 develop | ci-performance.yml |
| Push | Push to main | deploy-docs.yml |
| Tag | Push tag (v*) | ci-release.yml |
| Schedule | 每日 UTC 0:00 | ci-nightly.yml |
| Manual | Workflow Dispatch | 所有工作流 |

### 分支策略

```
main (生产分支)
├─ 触发: ci-pr.yml, ci-build-matrix.yml, ci-32bit.yml
├─ 合并来源: pre-release
└─ 保护规则: 需要 CI/CD 通过

develop (开发分支)
├─ 触发: ci-pr.yml, ci-build-matrix.yml, ci-32bit.yml, ci-performance.yml, ci-performance-tls.yml
├─ 合并来源: feature/*
└─ 保护规则: 需要 CI/CD 通过

pre-release (预发布分支)
├─ 触发: ci-build-matrix.yml
├─ 合并来源: main
└─ 保护规则: 需要 CI/CD 通过

feature/* (功能分支)
├─ 触发: ci-32bit.yml
├─ 合并来源: develop
└─ 保护规则: 无
```

---

## 并发控制

### 并发策略

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

### 工作流并发组

| 工作流 | 并发组 | 取消策略 |
|-------|-------|---------|
| ci-pr.yml | ci-pr-{PR_NUMBER} | 取消旧运行 |
| ci-build-matrix.yml | ci-build-matrix-{BRANCH} | 取消旧运行 |
| ci-32bit.yml | ci-32bit-{BRANCH} | 取消旧运行 |
| ci-performance.yml | ci-performance-{SHA} | 不取消 |
| ci-nightly.yml | ci-nightly-{CRON} | 不取消 |
| ci-release.yml | ci-release-{TAG} | 不取消 |

### 取消策略说明

- **PR 工作流**: 同一 PR 的新提交取消旧运行，节省资源
- **Push 工作流**: 同一分支的新推送取消旧运行
- **定时任务**: 不取消，确保每日测试都执行
- **发布工作流**: 不取消，确保完整发布流程

---

## 构建配置

### 编译器版本

| 平台 | 编译器 | 版本 |
|-----|-------|------|
| Linux | gcc | 12.x |
| Linux | g++ | 12.x |
| macOS | clang | Apple LLVM 14+ |
| Windows | MSVC | Visual Studio 2022 |

### CMake 版本

- **最低版本**: 3.10
- **推荐版本**: 3.25+
- **GitHub Actions**: 3.25.1

### 编译选项

#### Release 模式

```cmake
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_C_FLAGS="-O3 -DNDEBUG"
-DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"
```

#### Debug 模式

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-O0 -g -DDEBUG"
-DCMAKE_CXX_FLAGS="-O0 -g -DDEBUG"
```

#### 32 位模式

```cmake
-DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"
-DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"
-DEXE_LINKER_FLAGS="-m32"
-DSHARED_LINKER_FLAGS="-m32"
```

#### 覆盖率模式

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-O0 -g --coverage"
-DCMAKE_CXX_FLAGS="-O0 -g --coverage"
```

#### ASan 模式

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
```

---

## 测试策略

### 测试分类

#### 1. 单元测试

- **目的**: 测试单个函数和模块
- **工具**: Google Test
- **执行时间**: 1-2 分钟
- **覆盖率目标**: 80%+

#### 2. 集成测试

- **目的**: 测试模块间交互
- **工具**: wrk、ab
- **执行时间**: 3-5 分钟
- **测试场景**:
  - HTTP 请求/响应
  - WebSocket 连接
  - 静态文件服务
  - 路由匹配

#### 3. 性能测试

- **目的**: 监控性能指标
- **工具**: wrk、ab
- **执行时间**: 10-15 分钟
- **测试场景**:
  - 低/中/高并发
  - 持续负载
  - TLS 性能

#### 4. 压力测试

- **目的**: 验证稳定性
- **工具**: wrk
- **执行时间**: 35 分钟
- **测试场景**:
  - 500 并发持续 5 分钟
  - 1000 并发持续 30 秒

#### 5. 内存测试

- **目的**: 检测内存问题
- **工具**: AddressSanitizer
- **执行时间**: 20-25 分钟
- **检测内容**:
  - 内存泄漏
  - 使用后释放
  - 双重释放
  - 缓冲区溢出

### 测试执行

#### 快速测试 (PR 验证)

```bash
ctest --output-on-failure --timeout 60 -j$(nproc) \
  --label-regex "fast"
```

#### 完整测试 (Push 验证)

```bash
ctest --output-on-failure --timeout 180 -j$(nproc)
```

#### 覆盖率测试 (Nightly)

```bash
ctest --output-on-failure --timeout 120 -j$(nproc)
lcov --capture --directory . --output-file coverage.info
```

#### 内存测试 (Nightly)

```bash
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0 \
  ctest --output-on-failure --timeout 120 -j1
```

### 测试结果处理

#### 退出码处理

```bash
ctest --output-on-failure || {
  EXIT_CODE=$?
  if [ $EXIT_CODE -eq 8 ]; then
    echo "Some tests were skipped (expected when optional features are disabled)"
    exit 0
  else
    echo "Tests failed with exit code: $EXIT_CODE"
    exit $EXIT_CODE
  fi
}
```

退出码说明:

| 退出码 | 含义 |
|-------|------|
| 0 | 所有测试通过 |
| 1-7 | 测试失败 |
| 8 | 测试跳过 (预期行为) |

---

## 覆盖率管理

### 覆盖率目标

| 模块 | 目标覆盖率 | 当前覆盖率 | 状态 |
|-----|-----------|-----------|------|
| 整体 | 80% | 42.9% | ⚠️ 需提升 |
| uvhttp_static.c | 80% | 17.2% | ❌ 严重不足 |
| uvhttp_router.c | 80% | 32.3% | ⚠️ 需提升 |
| uvhttp_connection.c | 80% | 32.2% | ⚠️ 需提升 |
| uvhttp_server.c | 80% | 38.4% | ⚠️ 需提升 |
| uvhttp_request.c | 80% | 40.5% | ⚠️ 需提升 |
| uvhttp_tls.c | 80% | 38.3% | ⚠️ 需提升 |

### 覆盖率报告生成

```bash
# 生成覆盖率数据
lcov --capture --directory . --output-file coverage.info

# 过滤不需要的文件
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/deps/*' --output-file coverage.info
lcov --remove coverage.info '*/test/*' --output-file coverage.info

# 生成 HTML 报告
genhtml coverage.info --output-directory coverage-report

# 查看覆盖率
lcov --list coverage.info
```

### 覆盖率上传

```yaml
- name: Upload coverage to Codecov
  uses: codecov/codecov-action@v4
  with:
    files: ./build/coverage.info
    flags: nightly
    name: nightly-coverage
    fail_ci_if_error: false
```

### 覆盖率提升策略

1. **识别低覆盖率模块**
   - 使用 `lcov --list coverage.info` 查看
   - 优先提升覆盖率最低的模块

2. **添加测试用例**
   - 为未覆盖的代码路径添加测试
   - 使用链接时注入 (linker wrap) 实现 mock

3. **定期审查**
   - Nightly 测试生成覆盖率报告
   - 每周审查覆盖率变化

4. **质量门禁**
   - 新增代码覆盖率不低于 80%
   - 整体覆盖率目标 80%

---

## 性能监控

### 性能基线

#### 基线配置

```yaml
# config/performance-baseline.yml
baseline:
  low_concurrent:
    rps: 30000
    latency_p50: "2ms"
    latency_p95: "5ms"
    latency_p99: "10ms"
  
  medium_concurrent:
    rps: 30000
    latency_p50: "3ms"
    latency_p95: "8ms"
    latency_p99: "15ms"
  
  high_concurrent:
    rps: 30000
    latency_p50: "4ms"
    latency_p95: "12ms"
    latency_p99: "25ms"
  
  extreme_concurrent:
    rps: 25000
    latency_p50: "5ms"
    latency_p95: "20ms"
    latency_p99: "50ms"
```

### 性能退化检测

#### 检测规则

```python
def check_performance_degradation(current, baseline, threshold=0.05):
    """
    检查性能退化
    
    Args:
        current: 当前性能指标
        baseline: 基线性能指标
        threshold: 退化阈值 (默认 5%)
    
    Returns:
        bool: 是否退化
    """
    degradation = (baseline - current) / baseline
    return degradation > threshold
```

#### 告警规则

| 指标 | 退化阈值 | 告警级别 |
|-----|---------|---------|
| RPS | > 5% | ⚠️ Warning |
| RPS | > 10% | 🔴 Error |
| P95 延迟 | > 20% | ⚠️ Warning |
| P95 延迟 | > 50% | 🔴 Error |
| 错误率 | > 0.1% | 🔴 Error |

### 性能趋势分析

#### 趋势图生成

```python
import matplotlib.pyplot as plt

def plot_performance_trend(data):
    """
    生成性能趋势图
    
    Args:
        data: 历史性能数据
    """
    dates = [d['date'] for d in data]
    rps_values = [d['rps'] for d in data]
    
    plt.figure(figsize=(12, 6))
    plt.plot(dates, rps_values, marker='o')
    plt.title('Performance Trend (RPS)')
    plt.xlabel('Date')
    plt.ylabel('RPS')
    plt.xticks(rotation=45)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig('performance-trend.png')
```

#### 性能报告

```markdown
# Performance Report

## Summary

- **Date**: 2026-02-23
- **Commit**: 7fe7b27
- **Test**: #148

## Results

| Scenario | Baseline RPS | Current RPS | Change | Status |
|----------|--------------|-------------|--------|--------|
| Low Concurrent (10) | 30,000 | 31,151 | +3.8% | ✅ |
| Medium Concurrent (50) | 30,000 | 30,487 | +1.6% | ✅ |
| High Concurrent (100) | 30,000 | 31,409 | +4.7% | ✅ |
| Extreme Concurrent (500) | 25,000 | 28,234 | +12.9% | ✅ |

## Conclusion

No performance degradation detected. All metrics within acceptable range.
```

---

## 安全扫描

### CodeQL 扫描

#### 扫描配置

```yaml
- name: Run CodeQL analysis
  uses: github/codeql-action/analyze@v3
  with:
    languages: cpp
    queries: security-extended,security-and-quality
```

#### 查询类型

| 查询类型 | 说明 |
|---------|------|
| security-extended | 扩展安全查询 |
| security-and-quality | 安全和质量查询 |

### 依赖扫描

#### 依赖检查

```bash
# 检查依赖库版本
check_deps() {
  echo "Checking dependencies..."
  
  # libuv
  libuv_version=$(cat deps/libuv/VERSION)
  echo "libuv: $libuv_version"
  
  # mbedtls
  mbedtls_version=$(cat deps/mbedtls/VERSION)
  echo "mbedtls: $mbedtls_version"
  
  # llhttp
  llhttp_version=$(cat deps/cllhttp/VERSION)
  echo "llhttp: $llhttp_version"
}
```

#### 漏洞扫描

```yaml
- name: Run dependency check
  run: |
    # 使用 Dependency Check
    dependency-check --scan . --format JSON --out dependency-check-results.json
```

### 安全 Issue 自动创建

#### 工作流

```yaml
name: Security Issue Creator

on:
  security:
    types: [dependabot_alert]

jobs:
  create-issue:
    runs-on: ubuntu-latest
    permissions:
      issues: write
    
    steps:
      - name: Create security issue
        uses: actions/github-script@v6
        with:
          script: |
            const alert = context.payload.alert;
            const issue = await github.rest.issues.create({
              owner: context.repo.owner,
              repo: context.repo.repo,
              title: `🔒 Security: ${alert.security_vulnerability.package.name}`,
              body: `Security alert: ${alert.html_url}`,
              labels: ['security', 'dependabot']
            });
```

---

## 发布流程

### 分支策略

```
feature/* → develop → main → pre-release → release
```

### 合并规则

| 目标分支 | 来源分支 | CI/CD 要求 | 审查要求 |
|---------|---------|-----------|---------|
| develop | feature/* | ci-pr.yml 通过 | 1 人审查 |
| main | develop | ci-build-matrix.yml 通过 | 1 人审查 |
| pre-release | main | ci-build-matrix.yml 通过 | 1 人审查 |
| release | pre-release | 所有 CI/CD 通过 | 1 人审查 |

### 发布步骤

#### 1. develop → main

```bash
# 创建 PR
gh pr create --base main --head develop --title "Release $(date +%Y.%m.%d)"

# 等待 CI/CD 通过

# 合并 PR
gh pr merge --merge
```

#### 2. main → pre-release

```bash
# 创建 PR
gh pr create --base pre-release --head main --title "Pre-release $(date +%Y.%m.%d)"

# 等待 CI/CD 通过（完整测试）

# 合并 PR
gh pr merge --merge
```

#### 3. pre-release → release

```bash
# 创建 PR
gh pr create --base release --head pre-release --title "Release $(cat VERSION)"

# 等待 CI/CD 通过

# 合并 PR
gh pr merge --merge

# 创建 Git 标签
git tag -a v$(cat VERSION) -m "Release v$(cat VERSION)"
git push origin v$(cat VERSION)

# 自动触发 ci-release.yml
```

#### 4. GitHub Release

```bash
# ci-release.yml 自动创建 Release
# 包含:
# - 源码归档
# - 构建产物
# - Release Notes
```

### 发布检查清单

发布前必须确保:

- [ ] 所有 CI/CD 测试通过
- [ ] 代码覆盖率达标 (80%+)
- [ ] 性能测试通过
- [ ] 安全扫描无高危漏洞
- [ ] 文档已更新 (API 变更)
- [ ] CHANGELOG 已更新
- [ ] 版本号已更新 (VERSION 文件)
- [ ] Git 标签已创建

---

## 监控与告警

### 监控指标

| 指标 | 工具 | 阈值 | 告警 |
|-----|------|------|------|
| CI/CD 失败率 | GitHub Actions | > 5% | ✉️ Email |
| 测试失败率 | Google Test | > 0% | ✉️ Email |
| 性能退化 | wrk | > 5% | ✉️ Email |
| 覆盖率下降 | lcov | > 1% | ✉️ Email |
| 安全漏洞 | CodeQL | 高危 | 🔴 Issue |
| 依赖漏洞 | Dependabot | 高危 | 🔴 Issue |

### 告警渠道

| 渠道 | 类型 | 用途 |
|-----|------|------|
| Email | 通知 | CI/CD 失败、性能退化 |
| GitHub Issue | 追踪 | 安全漏洞、Bug 报告 |
| Slack | 实时 | CI/CD 状态更新 |
| GitHub Summary | 可视化 | PR 测试结果 |

### 告警规则

#### CI/CD 失败告警

```yaml
- name: Notify on failure
  if: failure()
  uses: actions/github-script@v6
  with:
    script: |
      await github.rest.repos.createDispatchEvent({
        owner: context.repo.owner,
        repo: context.repo.repo,
        event_type: 'ci-failure',
        client_payload: {
          workflow: context.workflow,
          run_number: context.runNumber,
          run_id: context.runId
        }
      });
```

#### 性能退化告警

```yaml
- name: Check performance degradation
  run: |
    python3 << 'EOF'
    import json
    
    with open('performance-results.json', 'r') as f:
        data = json.load(f)
    
    baseline_rps = 30000
    current_rps = data['results']['test_3']['rps']
    
    degradation = (baseline_rps - current_rps) / baseline_rps
    
    if degradation > 0.05:
        print(f"⚠️ Performance degradation: {degradation:.2%}")
        exit(1)
    EOF
```

---

## 最佳实践

### 1. 工作流设计

#### ✅ 推荐

```yaml
# 使用并发控制
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

# 使用矩阵策略
strategy:
  fail-fast: false
  matrix:
    config: [...]
```

#### ❌ 避免

```yaml
# 不要硬编码版本号
- uses: actions/checkout@v4.1.1

# 不要忽略错误
- run: ./run_tests.sh || true
```

### 2. 缓存策略

#### ✅ 推荐

```yaml
- uses: actions/cache@v3
  with:
    path: |
      deps/*/build
      build/CMakeCache.txt
    key: ${{ hashFiles('CMakeLists.txt') }}
```

#### ❌ 避免

```yaml
# 不要缓存所有内容
- uses: actions/cache@v3
  with:
    path: build/
```

### 3. 错误处理

#### ✅ 推荐

```yaml
# 检查测试退出码
- run: ctest --output-on-failure || {
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 8 ]; then
      echo "Tests skipped (expected)"
      exit 0
    else
      exit $EXIT_CODE
    fi
  }
```

#### ❌ 避免

```yaml
# 不要忽略所有错误
- run: ctest || true
```

### 4. 安全实践

#### ✅ 推荐

```yaml
# 使用最小权限
permissions:
  contents: read
  pull-requests: write

# 不要泄露密钥
- run: echo ${{ secrets.MY_SECRET }}
```

#### ❌ 避免

```yaml
# 不要给予过多权限
permissions:
  contents: write
  issues: write
  pull-requests: write
  deployments: write
```

### 5. 文档和注释

#### ✅ 推荐

```yaml
# 添加详细的注释
- name: Build 32-bit version (core library only)
  run: |
    # Install 32-bit toolchain
    sudo apt-get install gcc-multilib g++-multilib
    
    # Configure with 32-bit flags
    cmake -B build-32bit -DCMAKE_C_FLAGS="-m32"
```

#### ❌ 避免

```yaml
# 不要没有注释的复杂命令
- run: cmake -B build-32bit -DCMAKE_C_FLAGS="-m32" && cd build-32bit && make -j$(nproc) && file dist/lib/libuvhttp.a
```

---

## 附录

### A. 环境变量

| 变量 | 说明 | 示例 |
|-----|------|------|
| `GITHUB_SHA` | Commit SHA | `7fe7b27e29d41c06a0b539b6d742692ac055240d` |
| `GITHUB_REF` | 分支引用 | `refs/heads/develop` |
| `GITHUB_RUN_ID` | 运行 ID | `22299433830` |
| `GITHUB_RUN_NUMBER` | 运行编号 | `148` |
| `GITHUB_TOKEN` | GitHub Token | `***` |

### B. 常用命令

#### GitHub CLI

```bash
# 查看工作流运行
gh run list --workflow=ci-32bit.yml

# 查看工作流详情
gh run view 22299433830

# 监控工作流运行
gh run watch 22299433830

# 下载工作流产物
gh run download 22299433830
```

#### CMake

```bash
# 配置 Debug 模式
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 配置 Release 模式
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 配置 32 位模式
cmake -B build-32bit -DCMAKE_C_FLAGS="-m32"

# 构建项目
cmake --build build -j$(nproc)

# 运行测试
cd build && ctest --output-on-failure
```

#### 覆盖率

```bash
# 生成覆盖率数据
lcov --capture --directory build --output-file coverage.info

# 过滤覆盖率数据
lcov --remove coverage.info '/usr/*' --output-file coverage.info

# 生成 HTML 报告
genhtml coverage.info --output-directory coverage-report

# 查看覆盖率
lcov --list coverage.info
```

### C. 故障排查

#### 编译失败

1. 检查编译器版本
2. 检查 CMake 版本
3. 检查依赖库是否正确编译
4. 查看完整日志

#### 测试失败

1. 下载测试日志
2. 查看失败测试的详细信息
3. 本地重现问题
4. 添加调试信息

#### 性能退化

1. 对比基线数据
2. 检查最近的代码变更
3. 使用性能分析工具 (perf)
4. 优化热点代码

#### 缓存问题

1. 清除缓存
2. 更新缓存键
3. 检查缓存路径

### D. 参考资料

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [CMake 文档](https://cmake.org/documentation/)
- [Google Test 文档](https://google.github.io/googletest/)
- [lcov 文档](http://ltp.sourceforge.net/coverage/lcov.php)
- [wrk 文档](https://github.com/wg/wrk)
- [CodeQL 文档](https://codeql.github.com/docs/)

---

## 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|-----|------|---------|------|
| 2026-02-23 | 1.0 | 初始版本，完整 CI/CD 方案设计 | iFlow |

---

## 联系方式

如有问题或建议，请通过以下方式联系:

- **GitHub Issues**: https://github.com/adam-ikari/uvhttp/issues
- **Email**: [待添加]
- **Slack**: [待添加]

---

**文档结束**