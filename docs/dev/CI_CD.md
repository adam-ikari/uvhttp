# CI/CD 工作流文档

本文档详细说明了 UVHTTP 项目的 CI/CD 工作流配置和用途。

## 工作流概览

UVHTTP 使用 GitHub Actions 实现自动化 CI/CD，包含以下工作流：

| 工作流名称 | 触发条件 | 目的 | 运行时间 |
|---------|---------|------|---------|
| CI/CD - Build & Test Pipeline | push, pull_request | 快速构建和测试 | ~10分钟 |
| CI/CD - Performance Benchmark | schedule, pull_request | 性能基准测试 | PR: ~1分钟, 完整: ~10-15分钟 |
| CI/CD - Nightly Full Test | schedule (daily) | 完整测试套件 | ~20-30分钟 |
| CI/CD - Deploy Documentation | push to main/release | 部署文档 | ~5分钟 |

## 工作流详细说明

### 1. CI/CD - Build & Test Pipeline

**文件**: `.github/workflows/ci.yml`

**触发条件**:
- Push 到 `main`, `develop`, `release`, `release/*`, `feature/*` 分支
- Pull Request 到 `main` 或 `develop` 分支

**包含的 Job**:

#### ubuntu-build
- **平台**: Ubuntu Latest
- **功能**: 
  - 检出代码（包含子模块）
  - 配置 CMake（Release 模式）
  - 编译项目
  - 检查编译警告（零警告原则）
  - 上传构建产物

#### ubuntu-test-fast
- **平台**: Ubuntu Latest
- **功能**:
  - 下载构建产物
  - 运行快速单元测试
  - 上传测试日志

#### ubuntu-security
- **平台**: Ubuntu Latest
- **功能**:
  - 安装 cppcheck
  - 运行静态代码分析
  - 上传分析结果

#### ubuntu-performance
- **平台**: Ubuntu Latest
- **功能**:
  - 配置 CMake（启用示例程序）
  - 编译项目（包含示例）
  - 上传构建产物

#### ubuntu-performance-run
- **平台**: Ubuntu Latest
- **功能**:
  - 下载构建产物
  - 运行性能测试（wrk）
  - 验证服务器正常工作

**超时设置**: 30分钟

**并发控制**: 取消同一分支的旧运行

---

### 2. CI/CD - Performance Benchmark

**文件**: `.github/workflows/performance-benchmark.yml`

**触发条件**:
- 定时任务：每天 UTC 0:00（北京时间 8:00）
- 手动触发（workflow_dispatch）
- Pull Request 到 `main` 或 `develop` 分支

**包含的 Job**:

#### performance-benchmark
- **平台**: Ubuntu 22.04
- **功能**:
  - 检出代码（包含子模块）
  - 获取当前日期和时间戳
  - 安装依赖（cmake, build-essential, wrk, python3, matplotlib, numpy）
  - 配置系统性能优化（CPU 性能模式、禁用 swap、增加连接限制）
  - 编译项目（Release 模式，启用示例）
  - 运行性能测试

**性能测试场景**:

##### PR 快速测试（仅 PR 触发）
- **目的**: 快速验证 PR 是否引入性能回归
- **测试时长**: 5秒/测试
- **测试次数**: 1次
- **包含场景**:
  1. 低并发（10 连接）
  2. 中等并发（50 连接）
  3. 高并发（100 连接）
- **总耗时**: 约15秒

##### 完整性能测试（定时任务或手动触发）
- **目的**: 生成详细的性能基准报告，用于长期趋势分析
- **测试时长**: 10秒/测试
- **测试次数**: 3次取平均（低/中/高并发），2次取平均（压力/文件测试）
- **包含场景**:
  1. 低并发（10 连接，10 秒，3次）
  2. 中等并发（50 连接，10 秒，3次）
  3. 高并发（100 连接，10 秒，3次）
  4. 极端并发（500 连接，30 秒，2次）
  5. 超高并发（1000 连接，30 秒，2次）
  6. 持续负载（100 连接，60 秒，1次）
  7. 小文件下载（100 连接，10 秒，2次）
  8. 大文件下载（50 连接，10 秒，2次）
- **总耗时**: 约10-15分钟

**超时设置**:
- PR 触发: 15分钟
- 定时任务/手动触发: 60分钟

**输出**:
- `performance-results.json`: 性能测试结果（JSON 格式）
- `performance-report.md`: 性能基准测试报告（Markdown 格式）
- `performance.log`: 原始性能测试日志
- `server.log`: 服务器日志
- GitHub Release: 创建 `perf-{timestamp}` 标签的 release
- GitHub Summary: 显示关键性能指标

---

### 3. CI/CD - Nightly Full Test

**文件**: `.github/workflows/nightly.yml`

**触发条件**:
- 定时任务：每天 UTC 0:00（北京时间 8:00）
- 手动触发（workflow_dispatch）

**包含的 Job**:

#### nightly-build
- **平台**: Ubuntu Latest
- **功能**:
  - 检出代码（包含子模块）
  - 获取当前日期和时间戳
  - 编译 Release 版本（启用示例）
  - 运行快速测试
  - 运行慢速测试（超时 180 秒）
  - 编译 Debug 版本（启用覆盖率）
  - 运行覆盖率测试（超时 1800 秒）
  - 生成覆盖率报告（lcov）
  - 运行安全扫描（所有类型）
  - 运行性能测试
  - 编译 AddressSanitizer 版本
  - 运行内存测试（超时 120 秒）
  - 创建 nightly release
  - 清理旧的 nightly releases（保留最近 7 个）

**超时设置**: 120分钟

**输出**:
- 覆盖率报告（lcov 格式）
- Nightly Release（prerelease）
- 保留最近 7 个 nightly builds

---

### 4. CI/CD - Deploy Documentation

**文件**: `.github/workflows/deploy.yml`

**触发条件**:
- Push 到 `main` 或 `release/*` 分支
- 路径变更：`docs/**` 或 `docs/.vitepress/**`
- 手动触发（workflow_dispatch）

**包含的 Job**:

#### build
- **平台**: Ubuntu Latest
- **功能**:
  - 检出代码（包含子模块）
  - 设置 Node.js 20
  - 安装文档依赖（pnpm）
  - 运行安全审计（pnpm audit）
  - 构建文档（VitePress）
  - 验证构建输出
  - 部署到 GitHub Pages

**权限**:
- `contents: read`
- `pages: write`
- `id-token: write`

**并发控制**: 取消旧的部署

**输出**:
- 部署到 GitHub Pages（https://adam-ikari.github.io/uvhttp/）

---

## 可复用工作流

项目使用可复用工作流（Reusable Workflows）来减少重复代码：

### 1. build.yml

**路径**: `.github/workflows/reusable/build.yml`

**功能**: 编译项目

**参数**:
- `os`: 操作系统（ubuntu-latest, macos-latest, windows-latest）
- `build-type`: 构建类型（Debug, Release, RelWithDebInfo）
- `enable-examples`: 是否编译示例程序（true/false）
- `enable-coverage`: 是否启用覆盖率（true/false）
- `sanitizer`: Sanitizer 类型（address, thread, undefined, none）
- `cache-key`: 缓存键

**输出**:
- 上传构建产物

---

### 2. test.yml

**路径**: `.github/workflows/reusable/test.yml`

**功能**: 运行测试

**参数**:
- `os`: 操作系统
- `build-type`: 构建类型
- `test-type`: 测试类型（fast, slow, coverage, memory）
- `timeout`: 超时时间（秒）

**输出**:
- 上传测试日志
- 上传覆盖率报告（如果 test-type=coverage）

---

### 3. security.yml

**路径**: `.github/workflows/reusable/security.yml`

**功能**: 安全扫描

**参数**:
- `scan-type`: 扫描类型（all, cppcheck, clang-tidy）

**输出**:
- 上传扫描结果

---

## 复合动作（Composite Actions）

项目使用复合动作来封装常用操作：

### 1. setup-build

**路径**: `.github/actions/setup-build`

**功能**: 设置构建环境

### 2. run-tests

**路径**: `.github/actions/run-tests`

**功能**: 运行测试套件

### 3. cache-deps

**路径**: `.github/actions/cache-deps`

**功能**: 缓存依赖

---

## 依赖管理

### Dependabot

**文件**: `.github/dependabot.yml`

**功能**: 自动更新依赖

**配置**:
- **GitHub Actions**: 每周一 09:00 UTC 检查更新
- **npm (docs)**: 每周一 09:00 UTC 检查更新
- **限制**: 最多 10 个开放 PR
- **标签**: dependencies, github-actions, npm, documentation

---

## 性能优化

### 缓存策略

项目使用多层缓存来加速 CI/CD：

1. **依赖缓存**: 缓存 CMake、npm、pip 依赖
2. **构建缓存**: 缓存编译产物
3. **ccache**: 使用 ccache 加速编译

### 并发控制

所有工作流都启用了并发控制，避免重复运行：

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

---

## 故障排查

### 常见问题

#### 1. 性能测试超时

**症状**: Performance Benchmark 工作流超时失败

**原因**: 
- PR 触发时运行完整测试（超过 15 分钟）
- 网络延迟导致测试时间过长

**解决方案**:
- 确保使用 PR 快速测试模式（3 个测试，每个 5 秒）
- 检查网络连接
- 增加 GitHub Actions runner 资源（如果可用）

#### 2. 文档部署失败

**症状**: Deploy Documentation 工作流失败

**原因**:
- Node.js 版本不匹配
- 依赖安装失败
- 构建输出目录不存在

**解决方案**:
- 检查 Node.js 版本（应为 20）
- 清理 node_modules 重新安装
- 验证 VitePress 配置

#### 3. Nightly 测试失败

**症状**: Nightly Full Test 工作流失败

**原因**:
- 超时（120 分钟）
- 内存测试失败（AddressSanitizer）
- 覆盖率测试失败

**解决方案**:
- 检查测试日志
- 修复内存泄漏
- 确保测试覆盖率不下降

---

## 最佳实践

### 1. 提交前检查

在提交代码前，确保：
- ✅ 本地编译通过
- ✅ 本地测试通过
- ✅ 没有编译警告
- ✅ 代码符合项目规范

### 2. PR 检查清单

创建 PR 时，确保：
- ✅ PR 标题清晰描述变更
- ✅ PR 描述包含变更说明
- ✅ 关联相关 Issue
- ✅ CI/CD 所有检查通过
- ✅ 代码审查完成
- ✅ 文档已更新（如需要）

### 3. 性能回归检测

- ✅ PR 中运行快速性能测试
- ✅ 对比基准性能指标
- ✅ 如果性能下降超过 10%，需要调查

### 4. 安全扫描

- ✅ 定期运行安全扫描
- ✅ 及时修复发现的安全问题
- ✅ 使用 Dependabot 自动更新依赖

---

## 监控和指标

### 关键指标

- **构建成功率**: 目标 > 95%
- **平均构建时间**: 目标 < 15 分钟（PR）
- **测试覆盖率**: 目标 > 80%
- **性能基准**: 监控 RPS 和延迟
- **安全扫描**: 零高危漏洞

### 查看指标

- **GitHub Actions**: https://github.com/adam-ikari/uvhttp/actions
- **GitHub Pages**: https://adam-ikari.github.io/uvhttp/
- **Nightly Releases**: https://github.com/adam-ikari/uvhttp/releases/tag/nightly-{timestamp}

---

## 相关文档

- [测试标准](TESTING_STANDARDS.md)
- [性能基准测试](PERFORMANCE_BENCHMARK.md)
- [性能测试标准](PERFORMANCE_TESTING_STANDARD.md)
- [开发计划](DEVELOPMENT_PLAN.md)
- [安全指南](SECURITY.md)

---

## 贡献指南

如需修改 CI/CD 配置：

1. **修改工作流文件**: 编辑 `.github/workflows/*.yml`
2. **测试配置**: 在 feature 分支测试配置
3. **代码审查**: 提交 PR 进行审查
4. **文档更新**: 更新本文档
5. **通知团队**: 重大变更需要通知团队

---

## 联系方式

如有问题或建议，请联系：
- **GitHub Issues**: https://github.com/adam-ikari/uvhttp/issues
- **GitHub Discussions**: https://github.com/adam-ikari/uvhttp/discussions

---

**最后更新**: 2026-01-26
**维护者**: UVHTTP Team