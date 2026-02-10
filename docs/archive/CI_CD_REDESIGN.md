# UVHTTP CI/CD 重新设计方案

## 1. 现状分析

### 1.1 当前 CI/CD 配置

| Workflow | 触发条件 | 主要任务 | 超时时间 |
|----------|---------|---------|---------|
| ci.yml | push (main/develop/release/feature) + PR | 构建、快速测试、安全扫描、性能测试 | 30分钟 |
| performance-benchmark.yml | schedule (daily) + PR + manual | 性能基准测试 | 15-60分钟 |
| nightly.yml | schedule (daily) + manual | 完整测试（覆盖率、内存泄漏、压力测试） | 120分钟 |
| deploy.yml | push (main/release/*) + docs 路径变化 | 文档部署 | 30分钟 |

### 1.2 当前产物清单

| 产物名称 | 格式 | 保留策略 | 用途 |
|---------|------|---------|------|
| build-ubuntu-latest-Release | 二进制文件 | 7天 | 构建产物，用于测试 |
| test-logs-ubuntu-fast | 日志文件 | 7天 | 测试日志 |
| cppcheck-results | XML | 7天 | 静态代码分析结果 |
| performance-results | JSON/MD | 90天 | 性能基准测试结果 |
| performance-report | MD | 90天 | 性能测试报告 |
| baseline-history | JSON | 90天 | 性能历史记录 |
| coverage-report | HTML/XML | 30天 | 代码覆盖率报告 |
| nightly-build | 二进制文件 | 7天 | 每日构建产物 |

### 1.3 存在的问题

1. **任务重叠**：ci.yml 和 performance-benchmark.yml 都包含性能测试
2. **产物冗余**：多个 workflow 重复上传相同类型的产物
3. **性能基线管理混乱**：
   - baseline-history.json 在代码库和产物中都有
   - 没有明确的基线更新策略
   - 缺少性能回归自动检测
4. **超时设置不合理**：部分任务超时时间过长或过短
5. **缺少性能趋势分析**：没有可视化的性能趋势图
6. **安全扫描不完整**：缺少依赖漏洞扫描
7. **通知机制缺失**：没有 PR 回归检测通知

---

## 2. CI/CD 架构设计

### 2.1 架构原则

1. **单一职责**：每个 workflow 负责一个明确的任务
2. **并行执行**：最大化任务并行度，缩短总执行时间
3. **差异化处理**：PR、push、schedule 触发使用不同的测试策略
4. **产物最小化**：只上传必要的产物，减少存储和传输开销
5. **自动化反馈**：自动检测性能回归并通知

### 2.2 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         GitHub Events                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │   Push   │  │    PR    │  │ Schedule │  │  Manual  │        │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘        │
└───────┼────────────┼────────────┼────────────┼─────────────────┘
        │            │            │            │
        ▼            ▼            ▼            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Workflow Router                             │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Branch Filter + Event Type → Workflow Selection         │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
        │            │            │            │
        ▼            ▼            ▼            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Workflows                                  │
│                                                                   │
│  ┌─────────────────────┐  ┌─────────────────────┐              │
│  │   ci-pr.yml         │  │   ci-push.yml       │              │
│  │   (PR 触发)         │  │   (Push 触发)       │              │
│  │                     │  │                     │              │
│  │  • 快速构建         │  │  • 完整构建         │              │
│  │  • 快速测试         │  │  • 完整测试         │              │
│  │  • 性能回归检测     │  │  • 安全扫描         │              │
│  │  • 代码质量检查     │  │  • 性能基准测试     │              │
│  │                     │  │                     │              │
│  │  超时: 20分钟       │  │  超时: 45分钟       │              │
│  └─────────────────────┘  └─────────────────────┘              │
│                                                                   │
│  ┌─────────────────────┐  ┌─────────────────────┐              │
│  │   ci-nightly.yml    │  │   deploy-docs.yml    │              │
│  │   (定时触发)        │  │   (文档部署)         │              │
│  │                     │  │                     │              │
│  │  • 完整构建         │  │  • 文档构建         │              │
│  │  • 完整测试         │  │  • 静态网站部署     │              │
│  │  • 覆盖率测试       │  │                     │              │
│  │  • 内存泄漏检测     │  │                     │              │
│  │  • 压力测试         │  │                     │              │
│  │  • 完整性能基准     │  │                     │              │
│  │                     │  │                     │              │
│  │  超时: 120分钟      │  │  超时: 15分钟       │              │
│  └─────────────────────┘  └─────────────────────┘              │
│                                                                   │
│  ┌─────────────────────┐  ┌─────────────────────┐              │
│  │   ci-release.yml    │  │   notify.yml        │              │
│  │   (发布触发)        │  │   (通知服务)         │              │
│  │                     │  │                     │              │
│  │  • 多平台构建       │  │  • PR 评论           │              │
│  │  • 发布测试         │  │  • Issue 创建       │              │
│  │  • 发布包生成       │  │  • Slack 通知       │              │
│  │  • 性能基线更新     │  │                     │              │
│  │                     │  │                     │              │
│  │  超时: 60分钟       │  │  超时: 5分钟        │              │
│  └─────────────────────┘  └─────────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
        │            │            │            │
        ▼            ▼            ▼            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Artifacts                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  Build Art.  │  │  Test Art.   │  │  Report Art. │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Performance Baseline                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  Baseline DB │  │  Trend Data  │  │  Reports     │         │
│  │  (JSON)      │  │  (JSON)      │  │  (MD/HTML)   │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 Workflow 职责划分

| Workflow | 触发条件 | 主要职责 | 执行频率 |
|----------|---------|---------|---------|
| ci-pr.yml | PR (opened/synchronize/reopened) | 快速验证 PR，检测性能回归 | 每次 PR 更新 |
| ci-push.yml | Push (main/develop/feature) | 完整 CI 流程，生成性能基准 | 每次 push |
| ci-nightly.yml | Schedule (daily) | 深度测试，覆盖率，内存检测 | 每日一次 |
| ci-release.yml | Tag (v*) | 多平台构建，发布准备 | 每次发布 |
| deploy-docs.yml | Push (main, docs change) | 文档构建和部署 | 文档更新时 |
| notify.yml | On completion | 通知服务（PR 评论、Issue） | 任务完成时 |

---

## 3. Workflow 详细配置

### 3.1 ci-pr.yml（PR 快速验证）

**触发条件**：
- PR 打开、同步、重新打开到 main 或 develop 分支

**任务流程**：
```
1. 并行执行：
   ├─ ubuntu-build (15分钟)
   ├─ code-quality-check (10分钟)
   └─ dependency-scan (5分钟)

2. 依赖 ubuntu-build：
   ├─ ubuntu-test-fast (10分钟)
   └─ performance-regression-check (10分钟)

3. 依赖所有任务：
   └─ generate-pr-summary (2分钟)
```

**超时时间**：20分钟

**产物**：
- `build-ubuntu-pr-{PR_NUMBER}` (保留3天)
- `test-logs-pr-{PR_NUMBER}` (保留3天)
- `performance-comparison-pr-{PR_NUMBER}` (保留7天)

**关键特性**：
- 使用快速测试集（跳过慢速测试）
- 性能回归检测（与基线对比）
- 代码质量检查（cppcheck, clang-tidy）
- 依赖漏洞扫描（使用 GitHub Dependabot）
- 自动在 PR 中添加评论

### 3.2 ci-push.yml（Push 完整验证）

**触发条件**：
- Push 到 main、develop、feature/* 分支

**任务流程**：
```
1. 并行执行：
   ├─ ubuntu-build (15分钟)
   ├─ macos-build (20分钟)
   ├─ windows-build (25分钟)
   ├─ code-quality-check (10分钟)
   └─ security-scan (15分钟)

2. 依赖所有构建：
   ├─ ubuntu-test-full (15分钟)
   ├─ macos-test-full (15分钟)
   └─ windows-test-full (15分钟)

3. 依赖 ubuntu-test-full：
   └─ performance-benchmark (30分钟)

4. 依赖所有任务：
   └─ generate-push-summary (3分钟)
```

**超时时间**：45分钟

**产物**：
- `build-{MATRIX_OS}-push-{GITHUB_SHA}` (保留7天)
- `test-logs-{MATRIX_OS}-push-{GITHUB_SHA}` (保留7天)
- `security-scan-results-{GITHUB_SHA}` (保留7天)
- `performance-benchmark-{GITHUB_SHA}` (保留30天)
- `baseline-update-{GITHUB_SHA}` (保留30天)

**关键特性**：
- 多平台构建和测试
- 完整安全扫描（CodeQL, cppcheck, clang-tidy）
- 完整性能基准测试
- 性能基线自动更新（main/develop 分支）
- 生成详细测试报告

### 3.3 ci-nightly.yml（每日深度测试）

**触发条件**：
- Schedule (UTC 0:00, 每天)
- Manual trigger

**任务流程**：
```
1. 并行执行：
   ├─ ubuntu-build-all (20分钟)
   ├─ code-quality-full (15分钟)
   └─ security-scan-full (20分钟)

2. 依赖 ubuntu-build-all：
   ├─ test-coverage (30分钟)
   ├─ test-memory (20分钟)
   ├─ test-stress (30分钟)
   └─ performance-full (45分钟)

3. 依赖 test-coverage：
   └─ generate-coverage-report (10分钟)

4. 依赖所有任务：
   └─ generate-nightly-report (10分钟)
```

**超时时间**：120分钟

**产物**：
- `coverage-report-{RUN_NUMBER}` (保留30天)
- `memory-test-results-{RUN_NUMBER}` (保留30天)
- `stress-test-results-{RUN_NUMBER}` (保留30天)
- `performance-full-{RUN_NUMBER}` (保留90天)
- `nightly-report-{RUN_NUMBER}` (保留90天)

**关键特性**：
- 代码覆盖率测试
- 内存泄漏检测（AddressSanitizer, Valgrind）
- 压力测试（持续高负载）
- 完整性能基准（8个场景，多次迭代）
- 生成性能趋势报告
- 创建 nightly release

### 3.4 ci-release.yml（发布构建）

**触发条件**：
- Push tag matching v* (如 v1.5.0)

**任务流程**：
```
1. 并行执行：
   ├─ ubuntu-release-build (20分钟)
   ├─ macos-release-build (25分钟)
   └─ windows-release-build (30分钟)

2. 依赖所有构建：
   ├─ release-test (20分钟)
   └─ package-generation (10分钟)

3. 依赖 release-test：
   └─ create-release (5分钟)

4. 依赖 create-release：
   └─ update-baseline (5分钟)
```

**超时时间**：60分钟

**产物**：
- `uvhttp-{REF_NAME}-linux-x86_64.tar.gz` (永久)
- `uvhttp-{REF_NAME}-macos-x86_64.tar.gz` (永久)
- `uvhttp-{REF_NAME}-windows-x86_64.zip` (永久)
- `release-report-{REF_NAME}.md` (永久)

**关键特性**：
- 多平台 Release 构建
- 发布包生成（包含库、头文件、示例、文档）
- 发布测试（确保发布质量）
- 创建 GitHub Release
- 更新官方性能基线（记录到代码库）

### 3.5 deploy-docs.yml（文档部署）

**触发条件**：
- Push 到 main 分支，且 docs/** 路径有变化
- Push 到 release/* 分支，且 docs/** 路径有变化

**任务流程**：
```
1. 文档构建 (10分钟)
2. 部署到 GitHub Pages (3分钟)
```

**超时时间**：15分钟

**产物**：
- 无（直接部署）

**关键特性**：
- 使用 VitePress 构建文档
- 自动部署到 GitHub Pages
- 支持多版本文档

### 3.6 notify.yml（通知服务）

**触发条件**：
- On workflow completion (ci-pr, ci-push, ci-nightly, ci-release)

**任务流程**：
```
1. 根据触发事件和结果执行：
   ├─ PR 评论（性能回归、测试失败）
   ├─ Issue 创建（严重问题）
   └─ Slack 通知（可选）
```

**超时时间**：5分钟

**产物**：
- 无

**关键特性**：
- 自动在 PR 中添加性能对比评论
- 创建 Issue 追踪严重问题
- 可选的 Slack 通知

---

## 4. 产物设计

### 4.1 产物分类

#### 4.1.1 构建产物（Build Artifacts）

| 产物名称 | 格式 | 命名规范 | 保留策略 | 大小限制 |
|---------|------|---------|---------|---------|
| 二进制文件 | tar.gz/zip | `uvhttp-{version}-{os}-{arch}.tar.gz` | 永久（Release）/ 7天 | 50MB |
| 库文件 | .a/.so/.dylib/.dll | `libuvhttp.{ext}` | 7天 | 10MB |
| 头文件 | .h | `include/*.h` | 7天 | 1MB |
| 示例程序 | 二进制 | `examples/*` | 7天 | 20MB |

#### 4.1.2 测试产物（Test Artifacts）

| 产物名称 | 格式 | 命名规范 | 保留策略 | 大小限制 |
|---------|------|---------|---------|---------|
| 测试日志 | .log | `test-{os}-{type}-{id}.log` | 7天 | 10MB |
| 测试结果 | .xml | `test-{os}-{type}-{id}.xml` | 7天 | 5MB |
| 覆盖率报告 | .xml/.html | `coverage-{id}.{ext}` | 30天 | 50MB |
| 内存测试报告 | .txt | `memory-test-{id}.txt` | 30天 | 20MB |
| 压力测试报告 | .json | `stress-test-{id}.json` | 30天 | 10MB |

#### 4.1.3 性能产物（Performance Artifacts）

| 产物名称 | 格式 | 命名规范 | 保留策略 | 大小限制 |
|---------|------|---------|---------|---------|
| 性能结果 | .json | `performance-{id}.json` | 90天 | 1MB |
| 性能报告 | .md | `performance-report-{id}.md` | 90天 | 500KB |
| 性能日志 | .log | `performance-{id}.log` | 90天 | 5MB |
| 基线历史 | .json | `baseline-history.json` | 90天 | 2MB |
| 性能趋势图 | .png/.svg | `trend-{metric}-{period}.png` | 90天 | 1MB |

#### 4.1.4 安全产物（Security Artifacts）

| 产物名称 | 格式 | 命名规范 | 保留策略 | 大小限制 |
|---------|------|---------|---------|---------|
| CodeQL 结果 | .sarif | `codeql-{id}.sarif` | 30天 | 10MB |
| cppcheck 结果 | .xml | `cppcheck-{id}.xml` | 30天 | 5MB |
| clang-tidy 结果 | .txt | `clang-tidy-{id}.txt` | 30天 | 5MB |
| 依赖扫描结果 | .json | `deps-scan-{id}.json` | 30天 | 1MB |

### 4.2 产物命名规范

#### 4.2.1 通用命名格式

```
{category}-{os}-{type}-{identifier}.{ext}
```

**参数说明**：
- `category`: 产物类别（build, test, performance, security）
- `os`: 操作系统（ubuntu, macos, windows）
- `type`: 类型（fast, full, coverage, memory, stress）
- `identifier`: 唯一标识符（run_id, pr_number, sha, tag）
- `ext`: 文件扩展名

#### 4.2.2 示例

```
# 构建产物
build-ubuntu-latest-Release-abc123.tar.gz
build-macos-latest-Release-def456.tar.gz

# 测试产物
test-ubuntu-fast-123.log
test-macos-full-456.xml
coverage-report-789.html

# 性能产物
performance-benchmark-20250127.json
performance-report-20250127.md
baseline-history.json
trend-rps-7days.png

# 安全产物
codeql-results-abc123.sarif
cppcheck-results-def456.xml
```

### 4.3 产物保留策略

| 产物类型 | PR 触发 | Push 触发 | Nightly 触发 | Release 触发 |
|---------|---------|-----------|-------------|-------------|
| 构建产物 | 3天 | 7天 | 30天 | 永久 |
| 测试日志 | 3天 | 7天 | 30天 | 永久 |
| 覆盖率报告 | - | - | 30天 | 永久 |
| 性能结果 | 7天 | 30天 | 90天 | 永久 |
| 安全扫描结果 | - | 7天 | 30天 | 永久 |
| 基线历史 | - | 30天 | 90天 | 永久 |

### 4.4 产物压缩策略

1. **日志文件**：使用 gzip 压缩（压缩率约 80%）
2. **测试结果**：使用 gzip 压缩（压缩率约 70%）
3. **覆盖率报告**：仅上传 HTML 报告，不压缩
4. **性能结果**：使用 gzip 压缩（压缩率约 60%）
5. **构建产物**：使用 tar.gz 或 zip 格式

---

## 5. 性能基线管理

### 5.1 基线存储方案

#### 5.1.1 存储位置

```
docs/performance/
├── baseline.json              # 当前官方基线（代码库）
├── baseline-history.json      # 基线历史（代码库，仅 release 分支）
└── archive/
    ├── baseline-2024-01.json  # 归档基线（按月归档）
    ├── baseline-2024-02.json
    └── ...
```

#### 5.1.2 基线结构

```json
{
  "version": "1.0.0",
  "updated_at": "2025-01-27T00:00:00Z",
  "commit": {
    "sha": "abc123...",
    "short_sha": "abc1234",
    "message": "feat: 性能优化",
    "branch": "main"
  },
  "environment": {
    "os": "Linux 6.14.11-2-pve",
    "runner": "ubuntu-22.04",
    "cpu": "AMD Ryzen 7 5800H",
    "cores": 16,
    "memory": "12GB",
    "compiler": "GCC 11.4.0",
    "build_type": "Release",
    "optimization": "-O2"
  },
  "scenarios": {
    "low_concurrent": {
      "rps": 17798,
      "latency_avg": 518,
      "latency_p50": 500,
      "latency_p95": 800,
      "latency_p99": 1600,
      "latency_p999": 3000,
      "transfer_rate": 1.75,
      "errors": 0
    },
    "medium_concurrent": {
      "rps": 17209,
      "latency_avg": 2790,
      "latency_p50": 2600,
      "latency_p95": 4500,
      "latency_p99": 8500,
      "latency_p999": 15000,
      "transfer_rate": 1.69,
      "errors": 0
    },
    "high_concurrent": {
      "rps": 16623,
      "latency_avg": 12200,
      "latency_p50": 11000,
      "latency_p95": 18000,
      "latency_p99": 40000,
      "latency_p999": 70000,
      "transfer_rate": 1.63,
      "errors": 0
    }
  },
  "thresholds": {
    "rps_warning": 0.10,
    "rps_failure": 0.10,
    "latency_warning": 0.10,
    "latency_failure": 0.20
  }
}
```

### 5.2 基线更新策略

#### 5.2.1 更新条件

| 触发条件 | 更新类型 | 更新位置 | 说明 |
|---------|---------|---------|------|
| Push 到 main | 临时基线 | 产物 | 用于 PR 对比 |
| Push 到 develop | 临时基线 | 产物 | 用于开发分支对比 |
| Release (v*) | 官方基线 | 代码库 | 记录到 docs/performance/baseline.json |
| Nightly | 历史记录 | 产物 | 用于趋势分析 |

#### 5.2.2 更新流程

**Push 到 main/develop**：
```
1. 运行性能测试
2. 生成性能结果
3. 与当前基线对比
4. 如果性能提升 > 5%，更新临时基线
5. 上传 baseline-update-{sha}.json 产物
```

**Release (v*)**：
```
1. 运行完整性能测试
2. 生成性能结果
3. 与当前基线对比
4. 如果性能提升 > 5%，更新官方基线
5. 归档旧基线到 docs/performance/archive/
6. 提交 baseline.json 到代码库
7. 创建 Git tag: baseline-v{version}
```

**Nightly**：
```
1. 运行完整性能测试
2. 生成性能结果
3. 添加到 baseline-history.json
4. 上传 baseline-history.json 产物
5. 生成性能趋势图
```

### 5.3 性能回归检测

#### 5.3.1 检测规则

```python
# 回归检测逻辑
def detect_regression(current, baseline):
    results = {
        'regressions': [],
        'improvements': [],
        'warnings': []
    }
    
    for scenario in current['scenarios']:
        baseline_scenario = baseline['scenarios'][scenario]
        current_scenario = current['scenarios'][scenario]
        
        # 吞吐量检测
        rps_change = (current_scenario['rps'] - baseline_scenario['rps']) / baseline_scenario['rps']
        if rps_change < -baseline['thresholds']['rps_failure']:
            results['regressions'].append({
                'scenario': scenario,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps'],
                'change': rps_change * 100,
                'severity': 'failure'
            })
        elif rps_change < -baseline['thresholds']['rps_warning']:
            results['warnings'].append({
                'scenario': scenario,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps'],
                'change': rps_change * 100,
                'severity': 'warning'
            })
        elif rps_change > 0.05:  # 5% 提升
            results['improvements'].append({
                'scenario': scenario,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps'],
                'change': rps_change * 100
            })
        
        # 延迟检测
        latency_change = (current_scenario['latency_p99'] - baseline_scenario['latency_p99']) / baseline_scenario['latency_p99']
        if latency_change > baseline['thresholds']['latency_failure']:
            results['regressions'].append({
                'scenario': scenario,
                'metric': 'latency_p99',
                'baseline': baseline_scenario['latency_p99'],
                'current': current_scenario['latency_p99'],
                'change': latency_change * 100,
                'severity': 'failure'
            })
        elif latency_change > baseline['thresholds']['latency_warning']:
            results['warnings'].append({
                'scenario': scenario,
                'metric': 'latency_p99',
                'baseline': baseline_scenario['latency_p99'],
                'current': current_scenario['latency_p99'],
                'change': latency_change * 100,
                'severity': 'warning'
            })
    
    return results
```

#### 5.3.2 回归处理

| 严重级别 | 行动 | 阻止合并 |
|---------|------|---------|
| failure | 创建 Issue，PR 评论，阻止合并 | ✅ |
| warning | PR 评论，建议改进 | ❌ |
| improvement | PR 评论，记录改进 | ❌ |

### 5.4 性能趋势分析

#### 5.4.1 趋势数据结构

```json
{
  "metric": "rps",
  "scenario": "high_concurrent",
  "period": "7days",
  "data_points": [
    {
      "timestamp": "2025-01-20T00:00:00Z",
      "value": 16500,
      "commit": "abc123"
    },
    {
      "timestamp": "2025-01-21T00:00:00Z",
      "value": 16600,
      "commit": "def456"
    }
  ],
  "statistics": {
    "min": 16500,
    "max": 16700,
    "avg": 16600,
    "trend": "up",  // up, down, stable
    "change_percent": 1.2
  }
}
```

#### 5.4.2 趋势图生成

使用 Python + Matplotlib 生成趋势图：

```python
import matplotlib.pyplot as plt
import json

def generate_trend_chart(data, output_file):
    timestamps = [dp['timestamp'] for dp in data['data_points']]
    values = [dp['value'] for dp in data['data_points']]
    
    plt.figure(figsize=(12, 6))
    plt.plot(timestamps, values, marker='o')
    plt.title(f"{data['metric']} - {data['scenario']} ({data['period']})")
    plt.xlabel('Time')
    plt.ylabel(data['metric'])
    plt.grid(True)
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    plt.close()
```

#### 5.4.3 趋势图清单

| 图表名称 | 指标 | 场景 | 周期 |
|---------|------|------|------|
| trend-rps-7days.png | RPS | high_concurrent | 7天 |
| trend-rps-30days.png | RPS | high_concurrent | 30天 |
| trend-rps-90days.png | RPS | high_concurrent | 90天 |
| trend-latency-7days.png | latency_p99 | high_concurrent | 7天 |
| trend-latency-30days.png | latency_p99 | high_concurrent | 30天 |
| trend-latency-90days.png | latency_p99 | high_concurrent | 90天 |

---

## 6. 实施步骤和迁移计划

### 6.1 实施阶段

#### 阶段 1：准备阶段（1-2天）

**任务**：
1. 创建新的 workflow 文件
2. 创建可重用 actions
3. 创建性能基线模板
4. 创建通知服务脚本

**交付物**：
- `.github/workflows/ci-pr.yml`
- `.github/workflows/ci-push.yml`
- `.github/workflows/ci-nightly.yml`
- `.github/workflows/ci-release.yml`
- `.github/workflows/deploy-docs.yml`
- `.github/workflows/notify.yml`
- `.github/actions/setup-build/action.yml`
- `.github/actions/run-tests/action.yml`
- `.github/actions/cache-deps/action.yml`
- `.github/actions/performance-check/action.yml`
- `scripts/performance_regression.py`
- `scripts/notify_pr.py`
- `scripts/generate_trend_chart.py`
- `docs/performance/baseline.json` (模板)

#### 阶段 2：测试阶段（3-5天）

**任务**：
1. 在 feature 分支测试新 workflow
2. 验证所有任务正常执行
3. 验证产物上传和下载
4. 验证性能回归检测
5. 验证通知服务

**测试清单**：
- [ ] ci-pr.yml 正常执行
- [ ] ci-push.yml 正常执行
- [ ] ci-nightly.yml 正常执行
- [ ] ci-release.yml 正常执行
- [ ] deploy-docs.yml 正常执行
- [ ] notify.yml 正常执行
- [ ] 所有产物正确上传
- [ ] 性能回归检测准确
- [ ] PR 评论正常显示
- [ ] Issue 正常创建

#### 阶段 3：灰度发布（1周）

**任务**：
1. 合并新 workflow 到 develop 分支
2. 观察 develop 分支的 CI/CD 执行
3. 收集反馈和问题
4. 修复发现的问题

**监控指标**：
- Workflow 执行成功率
- 平均执行时间
- 产物上传成功率
- 性能回归检测准确率

#### 阶段 4：全面上线（1天）

**任务**：
1. 合并到 main 分支
2. 删除旧的 workflow 文件
3. 更新文档
4. 通知团队成员

**删除文件**：
- `.github/workflows/ci.yml`
- `.github/workflows/performance-benchmark.yml`
- `.github/workflows/nightly.yml`

### 6.2 回滚计划

如果新 CI/CD 系统出现严重问题：

1. **立即回滚**：
   ```bash
   git revert <commit-hash>
   git push origin main
   ```

2. **恢复旧 workflow**：
   ```bash
   git checkout <old-commit> -- .github/workflows/ci.yml
   git checkout <old-commit> -- .github/workflows/performance-benchmark.yml
   git checkout <old-commit> -- .github/workflows/nightly.yml
   git push origin main
   ```

3. **通知团队**：通过 Slack/邮件通知所有团队成员

### 6.3 数据迁移

#### 6.3.1 历史基线迁移

```bash
# 从旧产物中提取基线历史
# 创建 docs/performance/baseline-history.json
# 合并所有历史基线数据
```

#### 6.3.2 产物迁移

```bash
# 下载所有旧产物
# 重新命名（符合新命名规范）
# 重新上传（可选，仅保留重要产物）
```

---

## 7. 潜在风险和应对措施

### 7.1 风险清单

| 风险 | 影响 | 概率 | 应对措施 |
|-----|------|------|---------|
| Workflow 执行超时 | CI/CD 阻塞 | 中 | 优化任务并行度，增加超时时间 |
| 产物上传失败 | 数据丢失 | 低 | 重试机制，备份到 S3 |
| 性能回归检测误报 | 阻止有效 PR | 中 | 调整阈值，增加白名单 |
| 通知服务失败 | 信息不透明 | 低 | 日志记录，手动通知 |
| 基线更新冲突 | 数据不一致 | 低 | 使用 Git lock，串行化更新 |
| 多平台构建失败 | 发布延迟 | 中 | 逐步启用，先 Linux |
| 覆盖率测试超时 | Nightly 失败 | 中 | 优化测试，增加超时时间 |
| 依赖扫描误报 | 阻止合并 | 低 | 误报白名单，定期审查 |

### 7.2 监控和告警

#### 7.2.1 监控指标

| 指标 | 阈值 | 告警级别 |
|-----|------|---------|
| Workflow 失败率 | > 5% | Warning |
| Workflow 执行时间 | > 预期 20% | Warning |
| 产物上传失败率 | > 1% | Error |
| 性能回归检测误报率 | > 10% | Warning |
| 基线更新失败 | 任何 | Error |
| 通知服务失败率 | > 5% | Warning |

#### 7.2.2 告警渠道

- GitHub Actions 通知（默认）
- Slack 通知（可选）
- Email 通知（严重问题）

### 7.3 应急预案

#### 7.3.1 Workflow 持续失败

**症状**：同一 workflow 连续 3 次失败

**应对**：
1. 检查日志，确定失败原因
2. 如果是临时问题（网络、依赖），重试
3. 如果是代码问题，创建 Issue，通知团队
4. 如果是 workflow 配置问题，修复后立即重启

#### 7.3.2 性能回归检测误报

**症状**：性能回归检测误报率 > 10%

**应对**：
1. 分析误报原因
2. 调整阈值或增加白名单
3. 优化检测算法
4. 通知团队

#### 7.3.3 产物上传失败

**症状**：产物上传失败率 > 1%

**应对**：
1. 检查 GitHub Actions 配额
2. 优化产物大小
3. 增加重试机制
4. 备份到 S3（可选）

---

## 8. 附录

### 8.1 环境变量

| 变量名 | 说明 | 示例 |
|-------|------|------|
| `IS_PR` | 是否为 PR 触发 | `true` / `false` |
| `IS_MAIN` | 是否为 main 分支 | `true` / `false` |
| `IS_DEVELOP` | 是否为 develop 分支 | `true` / `false` |
| `IS_RELEASE` | 是否为 release 分支 | `true` / `false` |
| `BUILD_TYPE` | 构建类型 | `Release` / `Debug` |
| `TEST_TYPE` | 测试类型 | `fast` / `full` / `coverage` |
| `PERFORMANCE_THRESHOLD` | 性能阈值 | `0.10` |
| `BASELINE_FILE` | 基线文件路径 | `docs/performance/baseline.json` |

### 8.2 Secrets

| Secret 名称 | 说明 | 必需 |
|------------|------|------|
| `GITHUB_TOKEN` | GitHub API token | ✅ |
| `SLACK_WEBHOOK` | Slack 通知 webhook | ❌ |
| `AWS_ACCESS_KEY_ID` | AWS 访问密钥 | ❌ |
| `AWS_SECRET_ACCESS_KEY` | AWS 秘密密钥 | ❌ |
| `CODESIGNING_CERT` | 代码签名证书 | ❌ |

### 8.3 可重用 Actions

#### setup-build

```yaml
name: 'Setup Build Environment'
description: 'Setup build environment for different OS'
inputs:
  os:
    description: 'Operating system'
    required: true
runs:
  using: 'composite'
  steps:
    - name: Install dependencies (Ubuntu)
      if: inputs.os == 'ubuntu-latest'
      shell: bash
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake build-essential valgrind wrk lcov
    
    - name: Install dependencies (macOS)
      if: inputs.os == 'macos-latest'
      shell: bash
      run: |
        brew install cmake valgrind wrk
    
    - name: Install dependencies (Windows)
      if: inputs.os == 'windows-latest'
      shell: pwsh
      run: |
        choco install cmake
```

#### run-tests

```yaml
name: 'Run Tests'
description: 'Run tests with different types'
inputs:
  build-dir:
    description: 'Build directory'
    required: true
  test-type:
    description: 'Test type'
    required: true
  timeout:
    description: 'Test timeout'
    required: false
    default: '60'
  parallel:
    description: 'Number of parallel jobs'
    required: false
    default: '1'
outputs:
  status:
    description: 'Test status'
    value: {TEST_OUTPUT}
  total:
    description: 'Total tests'
    value: {TEST_OUTPUT}
  passed:
    description: 'Passed tests'
    value: {TEST_OUTPUT}
  failed:
    description: 'Failed tests'
    value: {TEST_OUTPUT}
  duration:
    description: 'Test duration'
    value: {TEST_OUTPUT}
runs:
  using: 'composite'
  steps:
    - name: Run tests
      id: test
      shell: bash
      working-directory: {INPUT}
      run: |
        TEST_ARGS="--output-on-failure -j{INPUT} --timeout {INPUT}"
        
        case "{INPUT}" in
          fast)
            TEST_ARGS="$TEST_ARGS --schedule-random"
            ;;
          slow)
            TEST_ARGS="$TEST_ARGS --schedule-random -L slow"
            ;;
          coverage)
            TEST_ARGS="$TEST_ARGS -T Coverage"
            ;;
          memory)
            TEST_ARGS="$TEST_ARGS -L memory"
            ;;
          stress)
            TEST_ARGS="$TEST_ARGS -L stress"
            ;;
          all)
            TEST_ARGS="$TEST_ARGS"
            ;;
        esac
        
        ctest $TEST_ARGS
        
        # Parse results
        TOTAL=$(ctest -N | grep "Total Tests:" | awk '{print $3}')
        PASSED=$(ctest -N | grep "Total Tests:" | awk '{print $3}')  # Simplified
        FAILED=$((TOTAL - PASSED))
        
        echo "status=success" >> $GITHUB_OUTPUT
        echo "total=$TOTAL" >> $GITHUB_OUTPUT
        echo "passed=$PASSED" >> $GITHUB_OUTPUT
        echo "failed=$FAILED" >> $GITHUB_OUTPUT
```

#### performance-check

```yaml
name: 'Performance Check'
description: 'Check performance regression'
inputs:
  current-results:
    description: 'Current performance results'
    required: true
  baseline-file:
    description: 'Baseline file path'
    required: true
  thresholds:
    description: 'Performance thresholds'
    required: false
    default: '{"rps_warning": 0.10, "rps_failure": 0.10, "latency_warning": 0.10, "latency_failure": 0.20}'
outputs:
  has-regression:
    description: 'Has regression'
    value: {CHECK_OUTPUT}
  has-improvement:
    description: 'Has improvement'
    value: {CHECK_OUTPUT}
  report:
    description: 'Regression report'
    value: {CHECK_OUTPUT}
runs:
  using: 'composite'
  steps:
    - name: Check performance
      id: check
      shell: bash
      run: |
        python3 << 'EOF'
        import json
        import sys
        
        # Load current results
        with open('{INPUT}', 'r') as f:
            current = json.load(f)
        
        # Load baseline
        with open('{INPUT}', 'r') as f:
            baseline = json.load(f)
        
        # Load thresholds
        thresholds = json.loads('{INPUT}')
        
        # Detect regression
        regressions = []
        improvements = []
        
        for scenario in current['test_scenarios']:
            scenario_name = scenario['name']
            if scenario_name not in baseline['scenarios']:
                continue
            
            baseline_scenario = baseline['scenarios'][scenario_name]
            current_scenario = scenario['results']
            
            # Check RPS
            rps_change = (current_scenario['rps']['value'] - baseline_scenario['rps']) / baseline_scenario['rps']
            if rps_change < -thresholds['rps_failure']:
                regressions.append({
                    'scenario': scenario_name,
                    'metric': 'rps',
                    'change': rps_change * 100
                })
            elif rps_change > 0.05:
                improvements.append({
                    'scenario': scenario_name,
                    'metric': 'rps',
                    'change': rps_change * 100
                })
        
        # Generate report
        report = {
            'regressions': regressions,
            'improvements': improvements
        }
        
        # Set outputs
        print(f"has-regression={len(regressions) > 0}")
        print(f"has-improvement={len(improvements) > 0}")
        print(f"report={json.dumps(report)}")
        
        # Exit with error if regression
        if len(regressions) > 0:
            sys.exit(1)
        EOF
        
        # Capture outputs
        echo "has-regression=$?" >> $GITHUB_OUTPUT
```

### 8.4 性能测试脚本

#### performance_regression.py

```python
#!/usr/bin/env python3
"""
Performance regression detection script
"""

import json
import sys
from typing import Dict, List, Any

def load_json_file(filepath: str) -> Dict[str, Any]:
    """Load JSON file"""
    with open(filepath, 'r') as f:
        return json.load(f)

def detect_regression(
    current: Dict[str, Any],
    baseline: Dict[str, Any],
    thresholds: Dict[str, float]
) -> Dict[str, Any]:
    """Detect performance regression"""
    results = {
        'regressions': [],
        'improvements': [],
        'warnings': []
    }
    
    for scenario in current.get('test_scenarios', []):
        scenario_name = scenario['name']
        if scenario_name not in baseline.get('scenarios', {}):
            continue
        
        baseline_scenario = baseline['scenarios'][scenario_name]
        current_scenario = scenario['results']
        
        # Check RPS
        rps_change = (
            current_scenario['rps']['value'] - baseline_scenario['rps']
        ) / baseline_scenario['rps']
        
        if rps_change < -thresholds['rps_failure']:
            results['regressions'].append({
                'scenario': scenario_name,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps']['value'],
                'change': rps_change * 100,
                'severity': 'failure'
            })
        elif rps_change < -thresholds['rps_warning']:
            results['warnings'].append({
                'scenario': scenario_name,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps']['value'],
                'change': rps_change * 100,
                'severity': 'warning'
            })
        elif rps_change > 0.05:
            results['improvements'].append({
                'scenario': scenario_name,
                'metric': 'rps',
                'baseline': baseline_scenario['rps'],
                'current': current_scenario['rps']['value'],
                'change': rps_change * 100
            })
        
        # Check latency
        if 'latency_p99' in current_scenario:
            latency_change = (
                current_scenario['latency_p99']['value'] - baseline_scenario['latency_p99']
            ) / baseline_scenario['latency_p99']
            
            if latency_change > thresholds['latency_failure']:
                results['regressions'].append({
                    'scenario': scenario_name,
                    'metric': 'latency_p99',
                    'baseline': baseline_scenario['latency_p99'],
                    'current': current_scenario['latency_p99']['value'],
                    'change': latency_change * 100,
                    'severity': 'failure'
                })
            elif latency_change > thresholds['latency_warning']:
                results['warnings'].append({
                    'scenario': scenario_name,
                    'metric': 'latency_p99',
                    'baseline': baseline_scenario['latency_p99'],
                    'current': current_scenario['latency_p99']['value'],
                    'change': latency_change * 100,
                    'severity': 'warning'
                })
    
    return results

def generate_report(results: Dict[str, Any]) -> str:
    """Generate regression report"""
    report = "# Performance Regression Report\n\n"
    
    if results['regressions']:
        report += "## ❌ Regressions Detected\n\n"
        for reg in results['regressions']:
            report += f"- **{reg['scenario']}**: {reg['metric']} "
            report += f"changed by {reg['change']:.2f}% "
            report += f"({reg['baseline']:.0f} → {reg['current']:.0f})\n"
        report += "\n"
    
    if results['warnings']:
        report += "## ⚠️ Warnings\n\n"
        for warn in results['warnings']:
            report += f"- **{warn['scenario']}**: {warn['metric']} "
            report += f"changed by {warn['change']:.2f}% "
            report += f"({warn['baseline']:.0f} → {warn['current']:.0f})\n"
        report += "\n"
    
    if results['improvements']:
        report += "## ✅ Improvements\n\n"
        for imp in results['improvements']:
            report += f"- **{imp['scenario']}**: {imp['metric']} "
            report += f"improved by {imp['change']:.2f}% "
            report += f"({imp['baseline']:.0f} → {imp['current']:.0f})\n"
        report += "\n"
    
    return report

def main():
    """Main function"""
    if len(sys.argv) < 3:
        print("Usage: performance_regression.py <current.json> <baseline.json>")
        sys.exit(1)
    
    current_file = sys.argv[1]
    baseline_file = sys.argv[2]
    
    # Load files
    current = load_json_file(current_file)
    baseline = load_json_file(baseline_file)
    
    # Default thresholds
    thresholds = {
        'rps_warning': 0.10,
        'rps_failure': 0.10,
        'latency_warning': 0.10,
        'latency_failure': 0.20
    }
    
    # Detect regression
    results = detect_regression(current, baseline, thresholds)
    
    # Generate report
    report = generate_report(results)
    print(report)
    
    # Exit with error if regression
    if results['regressions']:
        sys.exit(1)

if __name__ == '__main__':
    main()
```

---

## 9. 总结

本重新设计方案为 UVHTTP 项目提供了完整的 CI/CD 架构，包括：

1. **清晰的职责划分**：6 个 workflow 各司其职，避免任务重叠
2. **优化的执行效率**：最大化并行执行，缩短总执行时间
3. **完善的产物管理**：明确的命名规范和保留策略
4. **智能的性能基线管理**：自动更新、回归检测、趋势分析
5. **全面的安全措施**：安全扫描、权限控制、敏感信息保护
6. **良好的可维护性**：清晰的代码结构、充分的注释、易于扩展

通过实施本方案，UVHTTP 项目的 CI/CD 系统将更加高效、可靠、易维护，为项目的持续发展提供强有力的支持。