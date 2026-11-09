# Backlog and Daily Workflow

## Daily workflow

1. Check last night's `ci-nightly` result
   - Failure → claim the matching automatic issue (with the `bug` label)
   - All passing → continue with existing tasks

2. Pick the highest-priority open issue (P0 → P3)

3. Implement and submit a PR; close the issue after merge

## Current backlog

Latest issue list: https://github.com/adam-ikari/uvhttp/issues?q=is%3Aissue+is%3Aopen

## Quick commands

```bash
# Check last night's deep test run (coverage, stress, UBSan; failures open an issue automatically)
gh run list --workflow ci-nightly.yml --limit 1 --json conclusion

# List open issues by priority
gh issue list --state open --label P0,P1

# List the automatically created CI failure issues
gh issue list --label nightly --state open

# Create an issue
gh issue create --label "P1" --title "..."

# Close a finished issue
gh issue close <number>

# Check whether this week's retro file exists
ls docs/dev/weekly/
```
