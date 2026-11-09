# UVHTTP Solo AI Agile Development Guide

## Overview

UVHTTP runs on a solo AI agile model. A single AI developer (Claude) does the work, and automation (CI) enforces the process. GitHub Issues track tasks, and a change is done when its PR merges into `main`.

## Roles

| Role | Owner | Responsibility |
|------|--------|---------------|
| Product Owner | User | Set requirement priority, maintain the backlog, accept completed work |
| Developer | Claude | Implement backlog tasks, submit PRs |
| Scrum Master | Automation (CI) | Process gates: open an issue when tests fail, check the Friday retrospective |

The process has no dedicated Scrum Master. CI gates replace the ceremonies: a failure opens an issue automatically, a missing retrospective triggers a reminder, and nothing depends on the executor remembering.

## Weekly cycle

No 2-week sprints, no story point estimates, no stand-ups, review meetings or retrospectives.

| Day | Activity |
|----|------|
| Monday | Baseline check (`make test && make verify-memory-safety && make check-syntax`) plus the week's goals |
| Tuesday to Thursday | Pick tasks from Issues by priority (P0→P3), implement and submit PRs |
| Friday | Release when there is something to ship; write the retrospective (`docs/dev/weekly/<YYYY>-W<WW>.md`) |

See [development rhythm](development-rhythm.md) for the detailed cadence.

## Backlog structure

GitHub Issues hold the backlog. Each issue contains:

```markdown
## Title
[type] short description

## Description
what the task is

## Acceptance criteria
- [ ] condition 1
- [ ] condition 2
- [ ] test cases pass

## Technical notes
related modules, APIs, file paths

## Labels
type: feature / bug / refactor / docs / test / chore
priority: P0 (urgent) / P1 (high) / P2 (medium) / P3 (low)
```

## Issue types

| Type | Prefix | Description |
|------|--------|-------------|
| Feature | `feat:` | New feature work |
| Bug | `fix:` | Bug fix |
| Refactor | `refactor:` | Code refactoring |
| Docs | `docs:` | Documentation updates |
| Test | `test:` | Test work |
| Spec | `spec:` | Specification documents |
| Chore | `chore:` | Build and tooling |

## Workflow

```
Backlog → Ready → In Progress → Review → Done
```

### Backlog → Ready
- User confirms priority and acceptance criteria
- Requirements are clear

### Ready → In Progress
- Create a feature branch: `<type>/<short-description>`
- Example: `feat/add-compression`, `fix/memory-leak`

### In Progress → Review
- Implementation complete
- Local tests pass (`make test`)
- ASan reports nothing (`make verify-memory-safety`)
- Create a pull request

### Review → Done
- Code review passes
- CI pipeline passes
- Merge into `main`

## Branch strategy

```
main ────────●────────────●───────
             \            /
feature/     ●──●──●────●
                PR → Review → Merge
```

- `main`: production branch, always releasable
- `feat/*`: feature branches, created from main
- `fix/*`: fix branches
- `docs/*`: documentation branches

## Definition of Done

An issue is done only when:
- [ ] Implementation complete
- [ ] Unit tests pass (119/119)
- [ ] ASan build reports nothing
- [ ] Code review passes
- [ ] Spec documents updated (where applicable)
- [ ] Merged into main

## Priority definitions

| Priority | Response time | Description |
|--------|----------|------|
| P0 (Critical) | 24 hours | Production crash, data loss, security vulnerability |
| P1 (High) | 3 days | Major feature unavailable, severe performance regression |
| P2 (Medium) | 2 weeks | Minor feature missing, non-critical bug |
| P3 (Low) | Open-ended | Improvement, optimization, docs, technical debt |

## Completed baseline

- ASan/UBSan memory safety validation (119/119)
- Docs de-AI pass (8 English files + 41 Chinese files)
- Website consistency fixes
- Project structure cleanup (122 files removed)
- SDD spec documents (14 files)
- CI/CD pipelines (4 workflows)

## Next phase

See [Backlog and daily workflow](sprint-backlog.md) for the current task list.
