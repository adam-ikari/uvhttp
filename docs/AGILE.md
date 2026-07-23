# UVHTTP 敏捷开发管理指南

## 概述

UVHTTP 采用敏捷开发（Scrum 风格）管理项目，以 2 周为一个 Sprint，通过
GitHub Issues + Projects 跟踪进度，确保每个迭代都有可交付的增量。

## 角色

| 角色 | 职责 |
|------|------|
| **Product Owner** | 定义需求优先级，维护 Product Backlog，验收完成的工作 |
| **Scrum Master** | 保障流程顺畅，消除阻碍，主持 Sprint 仪式 |
| **Development Team** | 实现 Sprint Backlog 中的任务，自组织完成工作 |

## Sprint 周期（2 周）

```
第 1 天：Sprint Planning
第 1-10 天：Development
第 10 天：Sprint Review + Retrospective
```

### Sprint Planning

- 从 Product Backlog 中选取优先级最高的任务
- 团队估算工作量（Story Points: 1, 2, 3, 5, 8, 13）
- 承诺 Sprint 目标
- 输出：Sprint Backlog

### Daily Standup (每日站会)

- 昨天做了什么？
- 今天计划做什么？
- 有什么阻碍？

### Sprint Review

- 演示完成的功能
- 运行测试套件（91/91 通过）
- 检查规格文档一致性

### Sprint Retrospective

- 做得好的
- 做得不好的
- 改进计划

## Product Backlog 结构

Backlog 使用 GitHub Issues 管理，每条 Issue 包含：

```markdown
## 标题
[类型] 简短描述

## 描述
用户故事或技术任务描述

## 验收标准
- [ ] 条件 1
- [ ] 条件 2
- [ ] 测试用例通过

## 技术说明
相关模块、API、文件路径

## 标签
类型：feature / bug / refactor / docs / test / chore
优先级：P0 (紧急) / P1 (高) / P2 (中) / P3 (低)
状态：backlog / ready / in-progress / review / done
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
- Product Owner 确认优先级和验收标准
- 团队确认需求清晰

### Ready → In Progress
- 开发者认领任务
- 创建功能分支：`<type>/<short-description>`
- 例如：`feat/add-compression`, `fix/memory-leak`

### In Progress → Review
- 代码实现完成
- 本地测试通过（91/91）
- ASan 无发现
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
- [ ] 单元测试通过（91/91）
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

## Sprint 0 — 初始 Backlog

当前项目的初始 Sprint 0 已完成：

- ✅ ASan/UBSan 内存安全验证（91/91）
- ✅ 文档去 AI 味（英文 8 文件 + 中文 41 文件）
- ✅ 网站一致性修复
- ✅ 项目结构清理（122 个文件删除）
- ✅ SDD 规格文档（14 个文件）
- ✅ CI/CD 流水线（4 个工作流）

## 下一阶段

见 [Sprint Backlog](sprint-backlog.md) 获取当前 Sprint 的详细任务列表。