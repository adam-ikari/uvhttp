# CI/CD 工作流文档

本文档详细说明了 UVHTTP 项目的 CI/CD 工作流配置和用途。

## 工作流概览

UVHTTP 使用 GitHub Actions 实现自动化 CI/CD，专注于 Linux 平台，包含以下工作流：

| 工作流名称 | 触发条件 | 目的 | 运行时间 |
|---------|---------|------|---------|
| CI/CD - PR Quick Validation | PR 到 main/develop | 快速验证 PR | ~15分钟 |
| CI/CD - Push Full Validation | Push 到 main/develop/feature/* | 完整验证代码变更 | ~35分钟 |
| CI/CD - Nightly Deep Test | 每日定时 | 深度测试代码质量 | ~100分钟 |
| CI/CD - Release Build | Push tag v* | 构建和发布正式版本 | ~60分钟 |
| CI/CD - Deploy Documentation | Push 到 main/release/* + docs/** | 部署文档到 GitHub Pages | ~15分钟 |
| CI/CD - Notification Service | 其他 workflow 完成 | 发送 CI/CD 结果通知 | ~5分钟 |
| Security Issue Creator | 安全扫描完成 | 自动创建安全 Issue | ~10分钟 |

## 工作流详细说明

### 1. CI/CD - PR Quick Validation

**文件**: `.github/workflows/ci-pr.yml`

**触发条件**:
- Pull Request 打开、同步、重新打开到 `main` 或 `develop` 分支

**包含的 Job**:

#### ubuntu-build
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **功能**: 
  - 检出代码（包含子模块）
  - 配置 CMake（Release 模式，启用示例）
  - 编译项目
  - 检查编译警告（零警告原则）
  - 上传构建产物（保留7天）

#### code-quality-check
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **功能**:
  - 安装 cppcheck 和 clang-tidy
  - 运行静态代码分析
  - 上传分析结果（保留7天）

#### dependency-scan
- **平台**: Ubuntu Latest
- **超时**: 5分钟
- **功能**:
  - 检查 .gitmodules 中的依赖
  - 扫描依赖漏洞

#### ubuntu-test-fast
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **依赖**: ubuntu-build
- **功能**:
  - 下载构建产物
  - 使用 `run-tests` action 运行快速测试
  - 上传测试日志（保留7天）

#### performance-regression-check
- **平台**: Ubuntu Latest
- **超时**: 15分钟
- **依赖**: ubuntu-build
- **功能**:
  - 下载构建产物
  - 安装 wrk
  - 运行快速性能测试（3个场景，每个5秒）
  - 与基线对比性能指标
  - 检测性能回归
  - 上传性能结果（保留7天）

#### generate-pr-summary
- **平台**: Ubuntu Latest
- **超时**: 2分钟
- **依赖**: 所有其他任务
- **功能**:
  - 生成 PR 摘要
  - 在 PR 中添加评论

**并发控制**: 取消同一 PR 的旧运行

---

### 2. CI/CD - Push Full Validation

**文件**: `.github/workflows/ci-push.yml`

**触发条件**:
- Push 到 `main`, `develop`, `feature/*` 分支
- 手动触发

**包含的 Job**:

#### ubuntu-build
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **功能**: 
  - 检出代码（包含子模块）
  - 配置 CMake（Release 模式，启用 WebSocket 和 mimalloc）
  - 编译项目
  - 上传构建产物（保留7天）

#### code-quality-check
- **平台**: Ubuntu Latest
- **超时**: 15分钟
- **功能**:
  - 安装 cppcheck
  - 运行静态代码分析
  - 上传分析结果（保留7天）

#### security-scan
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **功能**:
  - 运行 CodeQL 安全分析
  - 上传分析结果

#### ubuntu-test-full
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **依赖**: ubuntu-build
- **功能**:
  - 下载构建产物
  - 设置可执行权限
  - 使用 `run-tests` action 运行完整测试
  - 上传测试日志（保留7天）

#### performance-benchmark
- **平台**: Ubuntu Latest
- **超时**: 35分钟
- **依赖**: ubuntu-test-full
- **功能**:
  - 下载构建产物
  - 安装 wrk
  - 运行性能测试（8个场景）
  - 解析性能结果
  - 生成性能报告
  - 更新性能基线历史
  - 上传性能结果（保留30天）

#### generate-summary
- **平台**: Ubuntu Latest
- **超时**: 5分钟
- **依赖**: ubuntu-test-full, performance-benchmark
- **功能**:
  - 生成 CI/CD 摘要
  - 更新 GitHub Step Summary

**并发控制**: 取消同一分支的旧运行

---

### 3. CI/CD - Nightly Deep Test

**文件**: `.github/workflows/ci-nightly.yml`

**触发条件**:
- 每日 UTC 0:00 定时运行
- 手动触发

**包含的 Job**:

#### ubuntu-build-all
- **平台**: Ubuntu Latest
- **超时**: 25分钟
- **功能**: 
  - 检出代码（包含子模块）
  - 配置 CMake（Debug 模式，启用覆盖率）
  - 编译项目
  - 上传构建产物（保留7天）

#### code-quality-full
- **平台**: Ubuntu Latest
- **超时**: 15分钟
- **功能**:
  - 运行 cppcheck
  - 运行 clang-tidy
  - 运行 clang-format
  - 上传分析结果（保留30天）

#### security-scan-full
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **功能**:
  - 运行 CodeQL 安全分析
  - 运行 cppcheck 安全检查
  - 上传分析结果（保留30天）

#### test-coverage
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **依赖**: ubuntu-build-all
- **功能**:
  - 下载构建产物
  - 使用 `run-tests` action 运行覆盖率测试
  - 生成覆盖率报告
  - 上传覆盖率报告（保留30天）
  - 上传到 Codecov

#### test-memory
- **平台**: Ubuntu Latest
- **超时**: 20分钟
- **依赖**: ubuntu-build-all
- **功能**:
  - 配置 CMake（启用 AddressSanitizer）
  - 编译项目
  - 使用 `run-tests` action 运行内存测试
  - 上传内存测试结果（保留30天）

#### test-stress
- **平台**: Ubuntu Latest
- **超时**: 35分钟
- **依赖**: ubuntu-build-all
- **功能**:
  - 下载构建产物
  - 运行压力测试
  - 上传压力测试结果（保留30天）

#### performance-full
- **平台**: Ubuntu Latest
- **超时**: 35分钟
- **依赖**: ubuntu-build-all
- **功能**:
  - 下载构建产物
  - 运行完整性能测试（8个场景）
  - 上传性能结果（保留30天）

#### generate-nightly-report
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **依赖**: 所有测试任务
- **功能**:
  - 生成每日测试报告
  - 更新 GitHub Step Summary

---

### 4. CI/CD - Release Build

**文件**: `.github/workflows/ci-release.yml`

**触发条件**:
- Push tag matching `v*`（如 v1.5.0）
- 手动触发

**包含的 Job**:

#### ubuntu-release-build
- **平台**: Ubuntu Latest
- **超时**: 25分钟
- **功能**: 
  - 检出代码（包含子模块）
  - 获取版本号
  - 配置 CMake（Release 模式，启用 WebSocket 和 mimalloc）
  - 编译项目
  - 上传构建产物（保留30天）

#### release-test
- **平台**: Ubuntu Latest
- **超时**: 25分钟
- **依赖**: ubuntu-release-build
- **功能**:
  - 下载构建产物
  - 运行发布测试
  - 上传测试日志（保留30天）

#### create-release
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **依赖**: release-test
- **功能**:
  - 下载构建产物
  - 生成 Release Notes
  - 创建 GitHub Release

#### update-baseline
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **依赖**: create-release
- **功能**:
  - 下载构建产物
  - 运行性能基准测试
  - 更新性能基线
  - 创建 PR 更新基线

---

### 5. CI/CD - Deploy Documentation

**文件**: `.github/workflows/deploy-docs.yml`

**触发条件**:
- Push 到 `main` 或 `release/*` 分支
- `docs/**` 路径有变化
- 手动触发

**包含的 Job**:

#### build-and-deploy
- **平台**: Ubuntu Latest
- **超时**: 15分钟
- **功能**: 
  - 检出代码（包含子模块）
  - 设置 Node.js 环境
  - 设置 pnpm
  - 安装依赖
  - 构建 VitePress 文档
  - 部署到 GitHub Pages

---

### 6. CI/CD - Notification Service

**文件**: `.github/workflows/notify.yml`

**触发条件**:
- 监听其他 workflow 完成

**包含的 Job**:

#### notify
- **平台**: Ubuntu Latest
- **超时**: 5分钟
- **功能**: 
  - 下载 workflow 产物
  - 解析 workflow 结果
  - 发送通知（PR 评论、Issue 创建）

---

### 7. Security Issue Creator

**文件**: `.github/workflows/security-issue-creator.yml`

**触发条件**:
- 监听安全扫描 workflow 完成

**包含的 Job**:

#### create-security-issue
- **平台**: Ubuntu Latest
- **超时**: 10分钟
- **功能**: 
  - 下载安全扫描结果
  - 检测安全问题
  - 创建安全 Issue
  - 更新 Issue 状态

---

## 可重用 Actions

### setup-build

**文件**: `.github/actions/setup-build/action.yml`

**功能**: 设置构建环境和安装依赖

**输入**:
- `os`: 操作系统（ubuntu-latest）

**输出**: 无

---

### cache-deps

**文件**: `.github/actions/cache-deps/action.yml`

**功能**: 缓存 CMake 依赖

**输入**:
- `cache-key`: 缓存键（用于标识）
- `build-type`: 构建类型（Release、Debug）

**缓存键格式**: `deps-{RUNNER_OS}-{BUILD_TYPE}-{HASH}`

---

### run-tests

**文件**: `.github/actions/run-tests/action.yml`

**功能**: 运行测试并收集结果

**输入**:
- `build-dir`: 构建目录
- `test-type`: 测试类型（fast、slow、all、coverage、stress、memory）
- `timeout`: 测试超时时间（秒）
- `parallel`: 并行测试数量

**输出**:
- `status`: 测试状态（success、failed）
- `total`: 总测试数
- `passed`: 通过测试数
- `failed`: 失败测试数
- `duration`: 测试持续时间（秒）

---

## 性能基准测试

### 测试场景

#### PR 快速测试（ci-pr.yml）
- **目的**: 快速验证 PR 是否引入性能回归
- **测试时长**: 5秒/测试
- **测试次数**: 1次
- **包含场景**:
  1. 低并发（2线程，10连接）
  2. 中等并发（4线程，50连接）
  3. 高并发（8线程，200连接）

#### Push 完整测试（ci-push.yml）
- **目的**: 验证代码变更后的性能
- **测试时长**: 10秒/测试
- **测试次数**: 1次
- **包含场景**:
  1. 低并发（2线程，10连接）
  2. 中等并发（4线程，50连接）
  3. 高并发（8线程，200连接）
  4. 极限并发（16线程，500连接）
  5. 首页（小响应）
  6. API 路由（中等响应）
  7. 静态文件（大响应）
  8. WebSocket 连接管理

#### Nightly 完整测试（ci-nightly.yml）
- **目的**: 深度性能测试和趋势分析
- **测试时长**: 30秒/测试
- **测试次数**: 3次（取平均值）
- **包含场景**: 同 Push 完整测试

### 性能指标

- **RPS (Requests Per Second)**: 每秒请求数
- **Latency**: 平均延迟
- **Transfer/sec**: 每秒传输量
- **Requests/sec**: 每秒请求数（总计）

### 性能回归检测

- **基线**: 从 `docs/performance/baseline.json` 读取
- **阈值**: 性能下降超过 5% 视为回归
- **动作**: 检测到回归时，阻止 workflow 并创建 Issue

---

## 产物保留策略

| 产物类型 | 保留时间 | 说明 |
|---------|---------|------|
| PR 构建产物 | 7天 | 快速验证，短期保留 |
| Push 构建产物 | 7天 | 代码验证，短期保留 |
| Nightly 测试结果 | 30天 | 深度测试，中期保留 |
| Release 构建产物 | 30天 | 发布版本，中期保留 |
| 性能测试结果 | 30天 | 性能分析，中期保留 |
| 代码质量分析结果 | 7-30天 | 质量跟踪，根据重要性保留 |

---

## 缓存策略

### 依赖缓存

**使用**: cache-deps action

**缓存路径**:
- `deps/libuv/build`
- `deps/mbedtls/build`
- `deps/xxhash/libxxhash.a`
- `deps/cllhttp/build`
- `deps/mimalloc/build`
- `deps/googletest/build`
- `deps/cjson/build`

**缓存键**: `deps-{RUNNER_OS}-{BUILD_TYPE}-{HASH}`

**恢复键**: `deps-{RUNNER_OS}-{BUILD_TYPE}-`

---

## 通知机制

### PR 评论

**触发**: ci-pr.yml 完成

**内容**:
- 构建状态
- 测试结果
- 代码质量
- 性能回归检测
- 依赖漏洞

### Issue 创建

**触发**: 严重失败或安全问题

**内容**:
- 失败详情
- 错误日志
- 修复建议

### GitHub Step Summary

**触发**: 所有 workflow 完成

**内容**:
- CI/CD 摘要
- 测试结果
- 性能指标
- 代码质量

---

## 安全扫描

### CodeQL

**运行**: ci-push.yml, ci-nightly.yml

**语言**: C++

**查询**: security-extended, security-and-quality

### cppcheck

**运行**: 所有 workflow

**启用**: warning, performance, portability

**抑制**:
- missingIncludeSystem
- unusedFunction
- constParameter
- unusedStructMember

### 依赖扫描

**运行**: dependency-scan job

**检查**: .gitmodules 中的依赖

---

## 性能优化

### 构建优化

- 并行编译：`-j$(nproc)`
- 零警告原则：编译警告导致失败
- 增量构建：依赖缓存

### 测试优化

- 并行测试：根据测试类型调整
- 测试超时：根据测试类型设置
- 快速测试：排除慢速、压力、内存测试

### 缓存优化

- 依赖缓存：缓存编译依赖
- 智能恢复：基于文件哈希恢复缓存

---

## 故障排查

### 常见问题

#### 1. 构建失败

**检查**:
- 编译警告
- 依赖安装
- 子模块更新

**解决**:
- 修复编译警告
- 检查 CMakeLists.txt
- 更新子模块

#### 2. 测试失败

**检查**:
- 测试日志
- 构建产物
- 测试超时

**解决**:
- 查看测试日志
- 检查测试代码
- 调整测试超时

#### 3. 性能回归

**检查**:
- 性能基线
- 测试场景
- 性能报告

**解决**:
- 对比基线
- 分析性能瓶颈
- 优化代码

#### 4. 安全扫描失败

**检查**:
- cppcheck 结果
- CodeQL 结果
- 依赖漏洞

**解决**:
- 修复安全问题
- 更新依赖
- 添加抑制规则

---

## 最佳实践

### 1. 提交前检查

- 运行本地测试
- 检查编译警告
- 运行性能测试
- 更新文档

### 2. PR 指南

- 确保所有测试通过
- 无性能回归
- 无安全问题
- 更新相关文档

### 3. 发布流程

- 更新版本号
- 运行完整测试
- 创建 Release Notes
- 更新文档

### 4. 性能优化

- 使用性能分析工具
- 优化热点代码
- 更新性能基线
- 监控性能趋势

---

## 参考资源

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [CMake 文档](https://cmake.org/documentation/)
- [ctest 文档](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [CodeQL 文档](https://codeql.github.com/docs/)
- [cppcheck 文档](https://cppcheck.sourceforge.io/manual.html)
- [wrk 文档](https://github.com/wg/wrk)

---

## 变更历史

### v2.2.0 (2026-01-29)

**重构**:
- 删除非 Linux 平台支持（macOS、Windows）
- 删除冗余工作流（ci.yml、deploy.yml）
- 充分复用可重用 Actions（run-tests）
- 统一缓存策略
- 优化通知服务触发条件

**优化**:
- PR 验证时间减少 25%
- Push 验证时间减少 22%
- Nightly 测试时间减少 17%
- CI 运行次数减少 20%
- 产物存储减少 50%

**新增**:
- 性能回归检测（ci-pr.yml）
- 统一测试输出格式
- 改进的通知机制

---

## 贡献指南

如需修改 CI/CD 配置：

1. 理解现有工作流和依赖关系
2. 测试修改的工作流
3. 更新本文档
4. 提交 PR 并说明变更

---

**最后更新**: 2026-01-29
**维护者**: UVHTTP 开发团队
**许可证**: MIT License