# UVHTTP 开发节奏

## 每周周期

AI 持续开发，24h 不间断。每周 5 个 Sprint 开发，周末 2 天作为 buffer 支配。

| 天 | Sprint | 活动 | 产出 |
|---|--------|------|------|
| 周一 | Sprint 1 | 开发 | 15-25 个 PR |
| 周二 | Sprint 2 | 开发 | 15-25 个 PR |
| 周三 | Sprint 3 | 开发 | 15-25 个 PR |
| 周四 | Sprint 4 | 开发 | 15-25 个 PR |
| 周五 | Sprint 5 | 测试 + 发布 | v2.x.y |
| 周六 | buffer | 处理溢出任务 | 按需 |
| 周日 | buffer | 处理溢出任务 | 按需 |

## 每日节奏

1. **确认日期**: 运行 `date "+%Y-%m-%d %A"` 确认今天是周几，判断当前 Sprint 阶段
2. **检查 daily-build 结果**: 如果有失败，创建 bug issue 加入当日 backlog
3. **从 Product Backlog 选取最高优先级任务**
4. **并行派发 subagent 执行**
5. **持续开发直到当日 Sprint 完成**

### 各 Sprint 确认要点

| 天 | Sprint | 确认要点 |
|---|--------|---------|
| 周一至周四 | Sprint 1-4 | 开发，选取 Product Backlog 中优先级最高的任务 |
| 周五 | Sprint 5 | 代码冻结，只做测试+发布，不引入新代码 |
| 周六/周日 | buffer | 检查是否有溢出任务，否则休息 |

## 当前日期映射

运行 `date "+%Y-%m-%d %A"` 确定当天活动。

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

- **daily-build** (每日凌晨): 构建 + 测试，失败自动创建 Issue
- **nightly** (每日): 深度测试 + 覆盖率 + ASan + UBSan
- **PR CI**: 每次 PR 自动验证
- **deploy**: 合并 main 自动部署网站