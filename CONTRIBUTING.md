# 贡献指南

感谢您对 UVHTTP 项目的关注！本文档将指导您如何为 UVHTTP 库的开发做出贡献。

## 开发流程

### 构建模式

UVHTTP 项目定义了三种构建模式（Release、Debug、Coverage），详细的构建模式规范请参考 [docs/zh/dev/BUILD_MODES.md](docs/zh/dev/BUILD_MODES.md)。

**重要提示**：性能测试必须使用 Release 模式，否则数据不准确。

### 分支策略

我们使用 Git Flow 工作流：

```
main (生产分支)
  ↑
  │ (合并)
  │
develop (开发分支)
  ↑
  │ (合并)
  │
feature/* (功能分支)
fix/* (修复分支)
```

**分支命名规范**：
- `feature/功能描述` - 新功能开发
- `fix/问题描述` - Bug 修复
- `refactor/重构描述` - 代码重构
- `docs/文档更新` - 文档更新
- `test/测试相关` - 测试相关

### 开发步骤

1. **创建功能分支**
   ```bash
   git checkout -b feature/your-feature-name develop
   ```

2. **开发和测试**
   - 编写代码
   - 添加单元测试
   - 确保所有测试通过
   - 遵循代码风格规范

3. **提交更改**
   ```bash
   git add .
   git commit -m "feat: 添加新功能描述"
   ```

4. **推送分支**
   ```bash
   git push origin feature/your-feature-name
   ```

5. **创建 Pull Request**
   - 在 GitHub 上创建 PR
   - 目标分支选择 `develop`
   - 填写 PR 描述模板
   - 等待代码审查

6. **合并 PR**
   - 至少需要 1 人审查批准
   - 所有 CI/CD 检查必须通过
   - 审查通过后合并到 develop

## 代码规范

### 提交信息格式

使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**类型（type）**：
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具链相关
- `perf`: 性能优化

**示例**：
```
feat(server): 添加 WebSocket 连接池支持

- 实现连接池管理
- 添加连接超时检测
- 优化连接复用逻辑

Closes #123
```

### 代码风格

- 使用 C11 标准
- 4 空格缩进
- K&R 风格大括号
- 函数命名：`uvhttp_module_action`
- 类型命名：`uvhttp_name_t`
- 常量命名：`UVHTTP_UPPER_CASE`

### 代码审查清单

在提交 PR 前，请确保：

- [ ] 代码遵循项目风格规范
- [ ] 添加了必要的单元测试
- [ ] 所有测试通过
- [ ] 没有编译警告
- [ ] 更新了相关文档
- [ ] 提交信息格式正确
- [ ] 没有引入新的安全漏洞
- [ ] 内存管理正确（使用 UVHTTP_MALLOC/UVHTTP_FREE）
- [ ] 错误处理完整

### 本项目易漏的检查点

以下每一条都在本项目造成过实际返工，均为「看代码看不出来、跑一遍也未必暴露」的类型：

- [ ] **测试真的会被执行到** — `test/integration/*.c` 会被 CMake 编译但**不会注册进 ctest**（只有 `add_test` 的测试才跑）。放进 integration 目录的测试 CI 永远不会执行。要进 CI 门禁必须放 `test/unit/`。
- [ ] **新增测试文件在裁剪构建下能编译** — `build-matrix` 的 `minimal` 配置关掉 WebSocket/HTTPS/static-files/LRU/compression，传递 include 会被裁掉。测试文件要**直接 include 自己调用的头**，不能依赖传递包含（`fuzz_*.c` 需要 `uvhttp_features.h` 同理）。
- [ ] **新增/改动 `.c/.h` 会被 format-check 门禁** — `format-check` 只对本 PR 变更的 C/H 文件跑 `clang-format --dry-run`，且是**全文件**检查（存量漂移也会挡）。`src/` 存量漂移已清理，但新改动务必 `clang-format -i`。
- [ ] **`.clang-format` 无重复键** — clang-format 18 拒绝解析含重复 mapping key 的配置（CI 用 18，本地可能是 14 而无法复现）。用工具校验而非凭印象；PyYAML 的 `safe_load` 默认**接受**重复键，是假阴性。
- [ ] **socket/loop 类测试的 fd 与 handle 释放** — 内存测试的 fd 泄漏 ASan 查不到（fd 不在 malloc 域）。fixture 内每条 `return`/提前退出路径都要 `close(fd)`；read 循环的退出条件要基于**实际收到的 body 长度**，而非 `Content-Length` 值混算，否则 TCP 分段投递时会读到截断 body 假失败。
- [ ] **CI 步骤顺序本身是正确性的一部分** — `upload-artifact`/`if: failure()` 只对**已执行过**的步骤生效。crash artifact 上传等收尾步骤必须排在**所有**会被 crash 打断的 Run 步骤之后，否则最需要留证时反而丢证据。
- [ ] **变异验证新测试有牙齿** — 造一个该测试本应捕获的 bug（如丢一个 iovec、破坏阈值边界），确认测试**变红**。一个改错了也照样绿的新测试，等于没有测试。
- [ ] **新增 fuzz harness 能在空 corpus 下命中回调** — 从空 corpus 起步跑短程，若 cov 长期停在个位数说明没触达目标函数。构造函数要「用真实 API 驱动」，不要自造不存在的接口（历史 `fuzz_request.c` 曾引用不存在的 `uvhttp_request_parse` 而从未编译）。

## 文档规范

### 文档双语要求

UVHTTP 项目要求所有非代码生成的文档必须提供中英双语版本，以支持全球贡献者和用户。

**详细的文档双语规范请参考**：[docs/zh/DOCUMENTATION_STANDARDS.md](docs/zh/DOCUMENTATION_STANDARDS.md)

### 文档分类

#### 1. 代码生成文档（无需双语）

**位置**: `docs/api/`

这些文档从代码注释自动生成，使用 Doxygen 工具，不需要人工翻译。

**示例**:
- `docs/api/defines/uvhttp_*.h.md`
- `docs/api/functions/uvhttp_*.h.md`

#### 2. 核心文档（必须双语）

这些文档是项目的重要组成部分，必须提供中英双语版本。

**根目录文档**：
- `docs/CHANGELOG.md` ↔ `docs/zh/CHANGELOG.md` ✅
- `docs/FAQ.md` ↔ `docs/zh/FAQ.md` ✅
- `docs/SECURITY.md` ↔ `docs/zh/SECURITY.md` ✅
- `docs/versions.md` ↔ `docs/zh/versions.md` ✅
- `docs/index.md` ↔ `docs/zh/index.md` ✅
- `docs/performance.md` ↔ `docs/zh/performance.md` ✅

**使用者文档（`/guide/`）**：
- `docs/guide/getting-started.md` ↔ `docs/zh/guide/getting-started.md` ✅
- 其他 guide 文档需要补充双语版本

**贡献者文档（`/dev/`）**：
- `docs/dev/ARCHITECTURE.md` ↔ `docs/zh/dev/ARCHITECTURE.md` ✅
- `docs/dev/BUILD_MODES.md` ↔ `docs/zh/dev/BUILD_MODES.md` ✅
- `docs/dev/DEVELOPER_GUIDE.md` ↔ `docs/zh/guide/DEVELOPER_GUIDE.md` ✅

### 术语规范

| 英文术语 | 中文术语 |
|---------|---------|
| Application Developer | 应用开发者 |
| Library Developer | 库开发者 |
| Contributor | 贡献者 |
| Maintainer | 维护者 |
| Build Mode | 构建模式 |
| Release Mode | Release 模式 |
| Debug Mode | Debug 模式 |
| Coverage Mode | Coverage 模式 |
| Performance Benchmark | 性能基准测试 |
| Unit Test | 单元测试 |
| Integration Test | 集成测试 |
| API Reference | API 参考 |
| Getting Started | 快速开始 |
| Architecture | 架构 |
| Middleware | 中间件 |

### 翻译原则

1. **准确性**: 确保技术术语翻译准确
2. **一致性**: 相同术语在不同文档中翻译一致
3. **可读性**: 翻译后的文档应符合目标语言的表达习惯
4. **完整性**: 确保所有内容都被翻译，包括代码注释

### 文件命名

- 英文文档：`docs/xxx/yyy.md`
- 中文文档：`docs/zh/xxx/yyy.md`

### 文档工作流程

#### 创建新文档

1. 创建英文文档：`docs/xxx/yyy.md`
2. 立即创建中文文档：`docs/zh/xxx/yyy.md`
3. 确保两个文档内容同步更新

#### 更新现有文档

1. 同时更新英文和中文文档
2. 使用 Git 提交时确保两个文件一起提交
3. 提交信息格式：`docs: 更新文档标题（双语）`

#### 代码注释

代码注释应使用英文，以便通过 Doxygen 生成英文 API 文档。

### 文档审查清单

在提交文档 PR 前，请确保：

- [ ] 英文文档和中文文档同时存在
- [ ] 两个文档的内容同步
- [ ] 术语翻译一致
- [ ] 格式正确
- [ ] 代码示例保持不变
- [ ] 链接地址正确
- [ ] 提交信息包含双语说明

## 测试要求

### 运行测试

```bash
cd build
make
ctest
```

### 测试覆盖率

- 目标覆盖率：80%
- 新功能必须包含测试
- Bug 修复必须包含回归测试

### 选择测试形态

不是所有测试都该放同一个目录。判断依据是**这条测试要被谁执行**：

| 形态 | 位置 | 谁执行 | 适用 |
|---|---|---|---|
| 单元测试 | `test/unit/*.cpp` | ctest → `ubuntu-test-fast` + `asan-gate` | 需要断言的逻辑、wire 级行为、回归测试 |
| fuzz harness | `test/fuzz/*.c` | 夜间 `ci-fuzz` | 吃不可信字节的解析/解码路径 |
| 集成测试 | `test/integration/*.c` | **无人执行**（仅编译） | 长驻服务进程、需外部 curl 驱动 |

**关键**：`test/integration/*.c` 虽然被 CMake 编译进 build，但**不注册进 ctest**，所以 CI 不会跑它。写单元测试时放 `test/unit/`，否则等于没写。

测试真实 socket 行为时用 gtest 起真实 server（`uv_tcp_getsockname` 取实际端口 + 阻塞 socket + `uv_run(UV_RUN_NOWAIT)` 泵循环），见 `test/unit/test_zerocopy_threshold_wire.cpp`。尺寸/阈值类参数从被测宏推导（如 `UVHTTP_ZEROCOPY_MIN_BODY`），不要硬编码——这样改配置无需改测试。

## 发布流程

### 版本发布

1. **创建 release 分支**
   ```bash
   git checkout -b release/v1.6.0 develop
   ```

2. **更新版本号**
   - 修改 `include/uvhttp.h` 中的版本号
   - 更新 `CHANGELOG.md`

3. **测试和验证**
   - 运行完整测试套件
   - 进行性能基准测试
   - 验证文档完整性

4. **创建 PR 到 main**
   - 目标分支：`main`
   - 需要至少 2 人审查批准
   - 所有检查必须通过

5. **合并和发布**
   - 合并到 main 后自动触发部署
   - 创建 Git 标签
   - 发布 GitHub Release

### 热修复

对于紧急修复：

1. 从 main 创建 hotfix 分支
2. 修复问题并测试
3. 合并回 main 和 develop
4. 立即发布补丁版本

## 问题报告

### Bug 报告

使用 GitHub Issues 报告 bug，请提供：

- 清晰的标题和描述
- 复现步骤
- 预期行为
- 实际行为
- 环境信息（OS、编译器版本等）
- 相关日志或错误信息

### 功能请求

使用 GitHub Issues 提交功能请求，请描述：

- 功能描述和用例
- 期望的行为
- 可能的实现方案
- 优先级（低/中/高）

## 行为准则

- 尊重所有贡献者
- 建设性批评
- 专注于代码质量
- 保持专业和友好

## 许可证

通过贡献代码，您同意您的贡献将在 MIT 许可证下发布。

## 联系方式

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues
- 讨论: https://github.com/adam-ikari/uvhttp/discussions

感谢您的贡献！