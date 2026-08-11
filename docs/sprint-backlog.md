# Backlog 与每日工作流

## 每日工作流

1. 检查 `ci-daily` 昨晚的结果
   - 有失败 → 认领对应的自动 issue（带 `bug` label）
   - 全部通过 → 继续推进原有任务

2. 从 open Issues 中选取优先级最高的任务（P0 → P3）

3. 实现并提交 PR，合并后关闭 Issue

## 当前 Backlog

最新 Issue 列表: https://github.com/adam-ikari/uvhttp/issues?q=is%3Aissue+is%3Aopen

## 速查命令

```bash
# 查看 ci-daily 结果
gh run list --workflow ci-daily.yml --limit 1 --json conclusion

# 查看夜间深度测试（nightly）结果
gh run list --workflow ci-nightly.yml --limit 1 --json conclusion

# 查看当前 open Issues（按优先级）
gh issue list --state open --label P0,P1

# 查看自动创建的 CI 失败 issue
gh issue list --label nightly --state open

# 创建新 Issue
gh issue create --label "P1" --title "..."

# 关闭完成的 Issue
gh issue close <number>

# 本周复盘是否已写
ls docs/dev/weekly/
```
