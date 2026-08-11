# UVHTTP 开发节奏

## 每周周期

单人 AI 开发，执行者是 Claude。不做 2 周 Sprint，按周循环推进。

| 天 | 活动 | 产出 |
|----|------|------|
| 周一 | 基线检查 + 定本周目标 | 基线通过；目标清单 |
| 周二至周四 | 从 Issues 选取任务实现 | 2-5 个主题 PR（按优先级从高到低） |
| 周五 | 收尾 + 发布（按需）+ 复盘 | 版本 tag（如有）；`docs/dev/weekly/` 复盘 |

周一基线：`make test && make verify-memory-safety && make check-syntax`。

## 每日节奏

1. **确认日期**: `date "+%Y-%m-%d %A"` 确定周几，判断阶段
2. **检查 CI 结果**: 查看 `ci-daily` 运行结果，有失败则认领对应的自动 issue
3. **从 GitHub Issues 选取最高优先级任务**（以 Issues 为准，sprint-backlog.md 为概览参考）
4. **实现并提交 PR**
5. **持续推进直到当日目标完成**

### 各天确认要点

| 天 | 确认要点 |
|----|---------|
| 周一 | 基线通过；从 Issues 选取 P0/P1 任务定本周目标 |
| 周二至周四 | 开发，从 GitHub Issues 选取 P0/P1 任务 |
| 周五 | 代码冻结；测试 + 发布（按需）+ 写复盘 |
| 周末 | 判断是否有溢出任务或紧急 bug，否则不开发 |

## CI 失败处理

CI 失败自动建 issue，无需人工记挂：

- **nightly 失败** → 自动创建/更新带 `nightly` label 的 `[nightly] CI 失败` issue
- **周五复盘缺失** → 自动创建带 `retro` label 的提醒 issue

开发者在每日节奏第 2 步认领这些 issue 并修复。

## 周五复盘

周五收尾时写复盘，产物为 `docs/dev/weekly/<YYYY>-W<WW>.md`（周数为 ISO 周号）。
缺失时 weekly-retro-check workflow 会自动提醒。

模板：

```markdown
# W<WW> 复盘（<YYYY>-MM-DD 至 MM-DD）

## 本周完成
- <PR/issue 简述>

## 未完成 / 遗留
- <原因>

## 下周目标
- <按优先级>

## CI 状态
- nightly / ci-daily 是否全绿；失败的修复状态

## 指标对比
- 测试：<数>/<数>；覆盖率：<百分比>
- 与上周对比：<变化>
```

## 周末判断标准

周末是否开发：
- **有溢出任务** → 继续开发
- **有紧急 bug（P0）** → 修复
- **以上都没有** → 不开发

## 任务管理

- **GitHub Issues** 是唯一任务来源，带优先级 label（P0-P3）
- **docs/sprint-backlog.md** 只作为概览参考，不单独维护
- 每天从 Issues 中选取优先级最高的任务

## 每日容量

每天 2-5 个主题 PR，按优先级从高到低推进。单 PR 对应单 Issue，PR 合并即
Issue 完成。

## 自动化

- **ci-daily**（每日凌晨，`.github/workflows/ci-daily.yml`）: 构建 + 测试
- **ci-nightly**（每日，`.github/workflows/ci-nightly.yml`）: 深度测试 + 覆盖率 + ASan + UBSan，失败自动建 issue
- **weekly-retro-check**（每周五，`.github/workflows/weekly-retro-check.yml`）: 复盘缺失提醒
- **PR CI**: 每次 PR 自动验证（ci-pr.yml）
- **deploy**: 合并 main 自动部署网站
