# Developer Documentation

Start here if you are contributing to the library rather than embedding it.

## Quick Start

```bash
git switch main && git pull --ff-only
git switch -c fix/your-fix

make build          # Debug build
make test           # ctest
make check-docs     # EN/ZH sync + internal links

git push -u origin fix/your-fix
gh pr create --base main
```

PR CI compiles and runs static checks but does **not** execute tests. Run
`make test` locally before opening a PR; the prerelease gate runs the full
matrix.

## Core Guides

| Document | Contents |
|---|---|
| [Developer Guide](../guide/DEVELOPER_GUIDE.md) | Environment, build commands, coding standards, memory management, review checklist |
| [Testing Guide](../guide/TESTING_GUIDE.md) | Test discovery, ctest, GoogleTest, libuv mock, sanitizers, coverage |
| [Build Modes](BUILD_MODES.md) | Release / Debug / Coverage semantics and flags |
| [CI/CD](CI_CD.md) | Every workflow, trigger, job, and local reproduction command |
| [Dependencies](DEPENDENCIES.md) | Vendored submodules and upgrade procedure |

## Architecture and Design

| Document | Contents |
|---|---|
| [Architecture](ARCHITECTURE.md) | Layer structure and module responsibilities |
| [Router Design Philosophy](ROUTER_DESIGN_PHILOSOPHY.md) | Routing model and trade-offs |
| [Router Search Modes](ROUTER_SEARCH_MODES.md) | Lookup strategies and their costs |
| [Header Include Order](HEADER_INCLUDE_ORDER.md) | Include ordering rules |
| [Forward Declarations](FORWARD_DECLARATION_AND_CIRCULAR_DEPENDENCY.md) | Breaking circular dependencies |
| [XXHash Integration](XXHASH_INTEGRATION.md) | Hashing dependency and usage |

## Quality and Process

| Document | Contents |
|---|---|
| [Testing Standards](TESTING_STANDARDS.md) | Test naming, structure, and validity requirements |
| [Testability Guide](TESTABILITY_GUIDE.md) | Designing code for deterministic tests |
| [Performance Testing Standard](PERFORMANCE_TESTING_STANDARD.md) | Benchmark construction and CI gate rules |
| [Markdown Style Guide](MARKDOWN_STYLE_GUIDE.md) | Documentation formatting |
| [Development Rhythm](../development-rhythm.md) | Weekly cadence and retro artifacts |
| [Release Strategy](../release-strategy.md) | Prerelease, benchmark gate, and promotion flow |

## Historical Documents

`DEVELOPMENT_PLAN.md`, `GLOBAL_VARIABLE_REFACTOR_PLAN.md`, and
`CI_CD_DESIGN.md` describe earlier plans or designs. They are kept for context
only; use the guides above for current behavior.
