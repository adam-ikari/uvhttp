# Sprint Backlog

## 每日工作流

每天早上重新规划当日 Sprint 的工作量：

1. 检查 `daily-build` 昨晚的结果
   - 如果有失败，创建 bug issue 并加入当日 backlog
   - 如果全部通过，继续推进原有任务

2. 从 Product Backlog 中选取优先级最高的任务
   - 估算工作量（6-7h = 1天容量）
   - 创建 Issue 并标记 `sprint-N` label

3. 开始当天开发

## 当前 Sprint

最新 Issue 列表: https://github.com/adam-ikari/uvhttp/issues?q=label%3Asprint-N

## 速查命令

```bash
# 查看 daily-build 结果
gh run list --workflow "daily-build" --limit 1 --json conclusion

# 查看当前 Sprint 的任务
gh issue list --label "sprint-3"

# 创建新的 Sprint Issue
gh issue create --label "test,P2,sprint-3" --title "..."

# 关闭完成的 Issue
gh issue close <number>
```