# UVHTTP Development Rhythm

## Weekly cycle

Solo AI development, with Claude as the executor. No 2-week sprints; the cycle is weekly.

| Day | Activity | Output |
|----|------|------|
| Monday | Baseline check + set the week's goals | Baseline passes; goal list |
| Tuesday to Thursday | Pick tasks from Issues and implement | 2-5 topic PRs, highest priority first |
| Friday | Wrap up + release (as needed) + retrospective | Version tag (if any); retrospective in `docs/dev/weekly/` |

Monday baseline: `make test && make verify-memory-safety && make check-syntax`.

## Daily rhythm

1. **Confirm the date**: run `date "+%Y-%m-%d %A"` to see the weekday and the phase
2. **Check CI results**: read the latest `ci-nightly` run and claim the matching automatic issue on failure
3. **Pick the highest-priority task from GitHub Issues** (Issues are authoritative; sprint-backlog.md is an overview)
4. **Implement and submit a PR**
5. **Keep going until the day's goal is met**

### Checks by day

| Day | Check |
|----|---------|
| Monday | Baseline passes; pick P0/P1 tasks from Issues and set the week's goals |
| Tuesday to Thursday | Develop; pick P0/P1 tasks from GitHub Issues |
| Friday | Code freeze; test + release (as needed) + retrospective |
| Weekend | Decide whether spillover tasks or urgent bugs exist; otherwise no development |

## CI failure handling

CI failures open issues automatically, so nobody has to remember:

- **Nightly failure** → creates or updates a `[nightly] CI 失败` issue with the `nightly` label
- **Missing Friday retrospective** → creates a reminder issue with the `retro` label

The developer claims these issues in step 2 of the daily rhythm and fixes them.

## Friday retrospective

Write the retrospective at Friday wrap-up. The artifact is `docs/dev/weekly/<YYYY>-W<WW>.md`, with the ISO week number. If it is missing, the weekly-retro-check workflow posts a reminder.

Template:

```markdown
# W<WW> 复盘（<YYYY>-MM-DD 至 MM-DD）

## 本周完成
- <PR/issue 简述>

## 未完成 / 遗留
- <原因>

## 下周目标
- <按优先级>

## CI 状态
- nightly 是否全绿；失败的修复状态

## 指标对比
- 测试：<数>/<数>；覆盖率：<百分比>
- 与上周对比：<变化>
```

## Weekend criteria

Whether to develop on the weekend:
- **Spillover tasks exist** → keep developing
- **An urgent bug (P0) exists** → fix it
- **Neither** → do not develop

## Task management

- **GitHub Issues** are the only task source, with priority labels (P0-P3)
- **docs/sprint-backlog.md** is an overview only and is not maintained separately
- Each day, pick the highest-priority task from Issues

## Daily capacity

2-5 topic PRs per day, highest priority first. One PR maps to one issue; merging the PR completes the issue.

## Automation

- **ci-nightly** (daily, `.github/workflows/ci-nightly.yml`): deep tests + coverage + UBSan, with automatic issues on failure
- **weekly-retro-check** (every Friday, `.github/workflows/weekly-retro-check.yml`): reminds when the retrospective is missing
- **PR CI**: validates every PR (ci-pr.yml)
- **deploy**: deploys the site when main changes
