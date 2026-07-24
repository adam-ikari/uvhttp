# Sprint 3 — Product Backlog

**Sprint 周期**: 2026-07-24 至 2026-08-06  
**Sprint 目标**: 提升测试覆盖率 + 完善 SDD 规格文档  
**Sprint 容量**: 18 Story Points

看板地址: https://github.com/adam-ikari/uvhttp/issues

## Sprint Backlog

### P1 — 必须完成

| ID | 标题 | 类型 | 估算 | 状态 | 链接 |
|----|------|------|------|------|------|
| #252 | Router 测试覆盖率提升至 80% (当前 62.9%) | test | 5 | backlog | [#252](https://github.com/adam-ikari/uvhttp/issues/252) |
| #253 | Connection 测试覆盖率提升至 70% (当前 42.8%) | test | 5 | backlog | [#253](https://github.com/adam-ikari/uvhttp/issues/253) |

### P2 — 应该完成

| ID | 标题 | 类型 | 估算 | 状态 | 链接 |
|----|------|------|------|------|------|
| #254 | WebSocket 集成自动化测试 | test | 5 | backlog | [#254](https://github.com/adam-ikari/uvhttp/issues/254) |
| #255 | 补充缺失的 SDD 规格文档 | docs | 3 | backlog | [#255](https://github.com/adam-ikari/uvhttp/issues/255) |
| #256 | 验证 UBSan 构建干净 | chore | 2 | backlog | [#256](https://github.com/adam-ikari/uvhttp/issues/256) |

## 工作流

每个 Issue 的流转：
```
Backlog → In Progress → Review → Done
```

**Backlog → In Progress**: 创建功能分支 `feat/fix/docs/test/chore/<description>`
**In Progress → Review**: 创建 PR，关联 Issue
**Review → Done**: PR 合并，CI 通过

## 速查命令

```bash
# 查看所有 open issues
gh issue list

# 查看 sprint 3 issues
gh issue list --label sprint-3

# 查看我的任务
gh issue list --assignee @me

# 开始一个任务
gh issue develop <number> --branch-name <type>/<description>

# 创建 PR 关联 issue
gh pr create --base main --title "<type>: <description>" --body "Closes #<number>"
```