# UVHTTP 单人 AI 敏捷开发指南

## 概述

UVHTTP 采用单人 AI 敏捷模型管理项目。执行者是单个 AI 开发者（Claude），
流程由自动化（CI）强制兜底。任务通过 GitHub Issues 跟踪，每个改动以 PR
合并到 `main` 为完成标志。

## 角色

| 角色 | 承担者 | 职责 |
|------|--------|------|
| Product Owner | 用户 | 定义需求优先级，维护 Backlog，验收完成的工作 |
| Developer | Claude | 实现 Backlog 中的任务，提交 PR |
| Scrum Master | 自动化（CI） | 流程门禁：测试失败建 issue、周五复盘检查 |

流程不设专职 Scrum Master。CI 门禁替代仪式：失败会自动建 issue，
复盘缺失会提醒，不依赖执行者自觉。

## 每周循环

不做 2 周 Sprint，不估 Story Points，无站会、评审会、复盘会。

| 天 | 活动 |
|----|------|
| 周一 | 基线检查（`make test && make verify-memory-safety && make check-syntax`）+ 定本周目标 |
| 周二至周四 | 从 Issues 按优先级（P0→P3）选取任务，实现并提交 PR |
| 周五 | 有可发布内容时发布；写复盘（`docs/dev/weekly/<YYYY>-W<WW>.md`） |

详细节奏见 [开发节奏](development-rhythm.md)。

## Backlog 结构

Backlog 使用 GitHub Issues 管理，每条 Issue 包含：

```markdown
## 标题
[类型] 简短描述

## 描述
任务描述

## 验收标准
- [ ] 条件 1
- [ ] 条件 2
- [ ] 测试用例通过

## 技术说明
相关模块、API、文件路径

## 标签
类型：feature / bug / refactor / docs / test / chore
优先级：P0 (紧急) / P1 (高) / P2 (中) / P3 (低)
```

## Issue 类型

| 类型 | 前缀 | 描述 |
|------|------|------|
| Feature | `feat:` | 新功能开发 |
| Bug | `fix:` | 错误修复 |
| Refactor | `refactor:` | 代码重构 |
| Docs | `docs:` | 文档更新 |
| Test | `test:` | 测试相关 |
| Spec | `spec:` | 规格文档 |
| Chore | `chore:` | 构建/工具链 |

## 工作流

```
Backlog → Ready → In Progress → Review → Done
```

### Backlog → Ready
- 用户确认优先级和验收标准
- 需求清晰

### Ready → In Progress
- 创建功能分支：`<type>/<short-description>`
- 例如：`feat/add-compression`, `fix/memory-leak`

### In Progress → Review
- 代码实现完成
- 本地测试通过（`make test`）
- ASan 无发现（`make verify-memory-safety`）
- 创建 Pull Request

### Review → Done
- 代码审查通过
- CI 流水线通过
- 合并到 `main` 分支

## 分支策略

```
main ────────●────────────●───────
             \            /
feature/     ●──●──●────●
                PR → Review → Merge
```

- `main`：生产分支，始终可发布
- `feat/*`：功能分支，从 main 创建
- `fix/*`：修复分支
- `docs/*`：文档分支

## 定义完成（Definition of Done）

一个 Issue 完成必须满足：

- [ ] 代码实现完成
- [ ] 单元测试通过（101/101）
- [ ] ASan 构建无发现
- [ ] 代码审查通过
- [ ] 规格文档已更新（如适用）
- [ ] 已合并到 main

## 优先级定义

| 优先级 | 响应时间 | 描述 |
|--------|----------|------|
| P0 (Critical) | 24 小时 | 生产环境崩溃、数据丢失、安全漏洞 |
| P1 (High) | 3 天 | 主要功能不可用、性能严重退化 |
| P2 (Medium) | 2 周 | 次要功能缺失、非关键 bug |
| P3 (Low) | 待定 | 改进、优化、文档、技术债务 |

## 已完成基线

- ASan/UBSan 内存安全验证（101/101）
- 文档去 AI 味（英文 8 文件 + 中文 41 文件）
- 网站一致性修复
- 项目结构清理（122 个文件删除）
- SDD 规格文档（14 个文件）
- CI/CD 流水线（4 个工作流）

## 下一阶段

见 [Backlog 与每日工作流](sprint-backlog.md) 获取当前任务列表。
