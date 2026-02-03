# UVHTTP 文档双语规范

## 概述

UVHTTP 项目要求所有非代码生成的文档必须提供中英双语版本，以支持全球开发者和用户。

## 文档分类

### 1. 代码生成文档（无需双语）

**位置**: `docs/api/`

这些文档从代码注释自动生成，使用 Doxygen 工具，不需要人工翻译。

**示例**:
- `docs/api/defines/uvhttp_*.h.md`
- `docs/api/functions/uvhttp_*.h.md`
- `docs/api/introduction.md`

### 2. 核心文档（必须双语）

这些文档是项目的重要组成部分，必须提供中英双语版本。

#### 根目录文档

| 英文文档 | 中文文档 | 说明 |
|---------|---------|------|
| `docs/CHANGELOG.md` | `docs/zh/CHANGELOG.md` | 变更日志 ✅ |
| `docs/FAQ.md` | `docs/zh/FAQ.md` | 常见问题 ✅ |
| `docs/SECURITY.md` | `docs/zh/SECURITY.md` | 安全指南 ⚠️ 缺少英文 |
| `docs/versions.md` | `docs/zh/versions.md` | 版本说明 ⚠️ 缺少英文 |
| `docs/index.md` | `docs/zh/index.md` | 文档首页 ✅ |
| `docs/performance.md` | `docs/zh/performance.md` | 性能文档 ✅ |

#### 使用者文档（`/guide/`）

| 英文文档 | 中文文档 | 说明 |
|---------|---------|------|
| `docs/guide/introduction.md` | `docs/zh/guide/introduction.md` | 介绍 ⚠️ 缺少中文 |
| `docs/guide/getting-started.md` | `docs/zh/guide/getting-started.md` | 快速开始 ✅ |
| `docs/guide/build.md` | `docs/zh/guide/build.md` | 构建指南 ⚠️ 缺少中文 |
| `docs/guide/TESTING_GUIDE.md` | `docs/zh/guide/TESTING_GUIDE.md` | 测试指南 ⚠️ 缺少中文 |
| `docs/guide/TUTORIAL.md` | `docs/zh/guide/TUTORIAL.md` | 教程 ⚠️ 缺少英文 |
| `docs/guide/websocket.md` | `docs/zh/guide/websocket.md` | WebSocket ⚠️ 缺少英文 |
| `docs/guide/RATE_LIMIT_API.md` | `docs/zh/guide/RATE_LIMIT_API.md` | 限流 API ⚠️ 缺少英文 |
| `docs/guide/STATIC_FILE_SERVER.md` | `docs/zh/guide/STATIC_FILE_SERVER.md` | 静态文件 ⚠️ 缺少英文 |
| `docs/guide/LIBUV_DATA_POINTER.md` | `docs/zh/guide/LIBUV_DATA_POINTER.md` | libvu 指针 ⚠️ 缺少英文 |
| `docs/guide/installation.md` | `docs/zh/guide/installation.md` | 安装指南 ⚠️ 缺少英文 |
| `docs/guide/first-server.md` | `docs/zh/guide/first-server.md` | 第一个服务器 ⚠️ 缺少英文 |

#### 贡献者文档（`/dev/`）

| 英文文档 | 中文文档 | 说明 |
|---------|---------|------|
| `docs/dev/ARCHITECTURE.md` | `docs/zh/dev/ARCHITECTURE.md` | 架构设计 ✅ |
| `docs/dev/BUILD_MODES.md` | `docs/zh/dev/BUILD_MODES.md` | 构建模式 ✅ |
| `docs/dev/DEVELOPER_GUIDE.md` | `docs/zh/guide/DEVELOPER_GUIDE.md` | 贡献者指南 ✅ |
| `docs/dev/CMAKE_IMPORTED_TARGETS_GUIDE.md` | `docs/zh/dev/CMAKE_IMPORTED_TARGETS_GUIDE.md` | CMake 导入 ⚠️ 缺少中文 |
| `docs/dev/CMAKE_TARGET_LINKING_GUIDE.md` | `docs/zh/dev/CMAKE_TARGET_LINKING_GUIDE.md` | CMake 链接 ⚠️ 缺少中文 |
| `docs/dev/PYTHON_TO_NODEJS_MIGRATION.md` | `docs/zh/dev/PYTHON_TO_NODEJS_MIGRATION.md` | 迁移指南 ✅ |

#### 项目文档

| 英文文档 | 中文文档 | 说明 |
|---------|---------|------|
| `docs/DEPENDENCIES.md` | `docs/zh/DEPENDENCIES.md` | 依赖说明 ⚠️ 缺少英文 |
| `docs/SECURITY.md` | `docs/zh/dev/SECURITY.md` | 安全规范 ✅ |
| `docs/README.md` | `docs/zh/README.md` | 文档说明 ⚠️ 缺少英文 |
| `docs/BRANCH_STRATEGY.md` | `docs/zh/BRANCH_STRATEGY.md` | 分支策略 ⚠️ 缺少英文 |
| `docs/ENGINEERING_STANDARDS.md` | `docs/zh/ENGINEERING_STANDARDS.md` | 工程规范 ⚠️ 缺少英文 |
| `docs/LIFECYCLE_DESIGN.md` | `docs/zh/LIFECYCLE_DESIGN.md` | 生命周期设计 ⚠️ 缺少英文 |
| `docs/MIGRATION_GUIDE.md` | `docs/zh/MIGRATION_GUIDE.md` | 迁移指南 ⚠️ 缺少英文 |
| `docs/RELEASE_CHECKLIST.md` | `docs/zh/RELEASE_CHECKLIST.md` | 发布检查清单 ⚠️ 缺少英文 |
| `docs/CI_CD.md` | `docs/zh/dev/CI_CD.md` | CI/CD 配置 ⚠️ 缺少英文 |
| `docs/DEVELOPMENT_PLAN.md` | `docs/zh/dev/DEVELOPMENT_PLAN.md` | 开发计划 ⚠️ 缺少英文 |
| `docs/ROADMAP.md` | `docs/zh/dev/ROADMAP.md` | 路线图 ⚠️ 缺少英文 |

### 3. 归档文档（可选双语）

**位置**: `docs/archive/`

这些是历史文档，可以不要求双语，但建议提供。

## 翻译规范

### 文件命名

- 英文文档：`docs/xxx/yyy.md`
- 中文文档：`docs/zh/xxx/yyy.md`

### 翻译原则

1. **准确性**: 确保技术术语翻译准确
2. **一致性**: 相同术语在不同文档中翻译一致
3. **可读性**: 翻译后的文档应符合中文表达习惯
4. **完整性**: 确保所有内容都被翻译，包括代码注释

### 术语对照表

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

### 格式要求

1. **Markdown 格式**: 保持与英文文档相同的 Markdown 格式
2. **代码示例**: 代码示例保持不变，只翻译注释和说明
3. **链接**: 链接地址保持不变，只修改链接文本
4. **图片**: 图片路径保持不变

## 工作流程

### 创建新文档

1. 创建英文文档：`docs/xxx/yyy.md`
2. 立即创建中文文档：`docs/zh/xxx/yyy.md`
3. 确保两个文档内容同步更新

### 更新现有文档

1. 同时更新英文和中文文档
2. 使用 Git 提交时确保两个文件一起提交
3. 提交信息格式：`docs: 更新文档标题（双语）`

### 代码注释

代码注释应使用英文，以便通过 Doxygen 生成英文 API 文档。

## 检查清单

在提交文档前，请确保：

- [ ] 英文文档和中文文档同时存在
- [ ] 两个文档的内容同步
- [ ] 术语翻译一致
- [ ] 格式正确
- [ ] 代码示例保持不变
- [ ] 链接地址正确
- [ ] 提交信息包含双语说明

## 工具支持

可以使用以下工具帮助翻译：

- Google Translate: 快速初稿
- DeepL: 更准确的翻译
- 人工校对: 确保技术术语准确

## 参考资料

- [Markdown 语法规范](https://commonmark.org/)
- [Doxygen 文档生成](https://www.doxygen.nl/)
- [技术文档写作指南](https://docs.microsoft.com/en-us/azure/devops/user-guide/tech-writing-basics)