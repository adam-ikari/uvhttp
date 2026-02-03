# 贡献指南

感谢您对 UVHTTP 项目的关注！本文档将指导您如何为项目做出贡献。

## 开发流程

### 构建模式规范

UVHTTP 项目定义了三种构建模式，开发时请根据场景选择合适的模式：

| 构建模式 | 用途 | 编译选项 | 适用程序 |
|----------|------|----------|----------|
| **Release** | 生产环境、性能测试 | `-O2 -DNDEBUG` | 所有 benchmark 程序、示例程序 |
| **Debug** | 开发调试、单元测试 | `-O0 -g` | 单元测试、测试程序 |
| **Coverage** | 代码覆盖率分析 | `-O0 --coverage` | 覆盖率测试 |

**重要提示**：
- ⚠️ **性能测试必须使用 Release 模式**，否则数据不准确（Debug 模式性能可能低 10-100 倍）
- ✅ 详细的构建模式规范请参考 [BUILD_MODES.md](BUILD_MODES.md)

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