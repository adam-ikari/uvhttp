# UVHTTP 开发节奏

## 每周周期

AI 持续开发，24h 不间断。每周 5 个 Sprint 开发，周末按需。

| 天 | Sprint | 活动 | 产出 |
|---|--------|------|------|
| 周一 | Sprint 1 | 开发 | 15-25 个 PR |
| 周二 | Sprint 2 | 开发 | 15-25 个 PR |
| 周三 | Sprint 3 | 开发 | 15-25 个 PR |
| 周四 | Sprint 4 | 开发 | 15-25 个 PR |
| 周五 | Sprint 5 | 测试 + 发布 | 版本发布 |
| 周六/周日 | — | 按需 | 仅溢出任务或紧急 bug |

Sprint 编号仅用于标识开发周期，不与版本号绑定。版本号由变更内容决定（patch/minor/major）。

## 每日节奏

1. **确认日期**: `date "+%Y-%m-%d %A"` 确定周几，判断 Sprint 阶段
2. **检查 daily-build 结果**: 查看 CI 日志，有失败则创建 Issue
3. **从 GitHub Issues 选取最高优先级任务**（以 Issues 为准，sprint-backlog.md 为概览参考）
4. **并行派发 subagent 执行**
5. **持续开发直到当日 Sprint 完成**

### 各 Sprint 确认要点

| 天 | Sprint | 确认要点 |
|---|--------|---------|
| 周一至周四 | Sprint 1-4 | 开发，从 GitHub Issues 选取 P0/P1 任务 |
| 周五 | Sprint 5 | 代码冻结，只做测试+发布，不引入新代码 |
| 周末 | — | 判断是否有溢出任务或紧急 bug，否则休息 |

## 周末判断标准

周末是否开发：
- **有 Sprint 溢出任务** → 继续开发
- **有紧急 bug（P0）** → 修复
- **以上都没有** → 不开发

## 任务管理

- **GitHub Issues** 是唯一任务来源，带 `sprint-N` label
- **docs/sprint-backlog.md** 只作为概览参考，不单独维护
- 每天从 Issues 中选取 `sprint-N` + `P1` 优先级最高的任务

## Sprint 容量

每个 Sprint 可并行派发多个 subagent，总容量 15-25 个 PR。

| 任务类型 | 平均耗时 | 可并行数 |
|---------|---------|---------|
| docs | 0.5-1h | 6-12 个 |
| fix | 1-2h | 3-6 个 |
| feat | 1-1.5h | 4-6 个 |
| test | 0.5-1h | 6-12 个 |
| chore | 0.3-0.5h | 12-18 个 |

## 自动化

- **daily-build** (每日凌晨): 构建 + 测试，失败记录在 CI 日志中
- **nightly** (每日): 深度测试 + 覆盖率 + ASan + UBSan
- **PR CI**: 每次 PR 自动验证
- **deploy**: 合并 main 自动部署网站