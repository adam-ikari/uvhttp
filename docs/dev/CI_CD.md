# CI/CD Workflow Guide

This page describes the workflows that exist in `.github/workflows/` today. The
workflow files are the source of truth; update this page when a trigger or job
changes.

There is no `ci-push.yml` and no `develop` branch. All changes land on `main`
through pull requests, and releases are staged with tags and GitHub
prereleases.

## Workflow Map

| Workflow | Trigger | Purpose |
|---|---|---|
| `ci-pr.yml` | PR opened/synchronized/reopened against `main` | Compile, cppcheck, test-validity check, doc sync/links, build matrix. **Does not run tests.** |
| `ci-release-gate.yml` | GitHub release published (stable or prerelease), or manual dispatch | Build, fast tests, tested build matrix, ASan gate. Skips `nightly-*` tags. |
| `ci-benchmark.yml` | Release published (non-nightly), or manual dispatch | Same-runner paired benchmark regression gate. |
| `ci-nightly.yml` | Daily 00:00 UTC, or manual dispatch | All-on build, security scan, coverage, stress, UBSan, failure issue. |
| `ci-fuzz.yml` | Daily 03:17 UTC, or manual dispatch | libFuzzer harness runs. |
| `weekly-retro-check.yml` | Friday 11:00 UTC, or manual dispatch | Checks that `docs/dev/weekly/<YYYY>-W<WW>.md` exists. |
| `deploy-docs.yml` | Push to `main` touching docs paths, or manual dispatch | Doxygen + API Markdown + VitePress build, deploy to GitHub Pages. |

## Pull Requests (`ci-pr.yml`)

Four gates run on every PR:

- **`ubuntu-build`** — Release build with `BUILD_EXAMPLES=ON`, zero-warning
  check, artifact upload.
- **`code-quality-check`** — cppcheck over `src/` with `--error-exitcode=1`,
  plus `scripts/ci/check_test_validity.py --strict` to reject always-green
  tests.
- **`doc-sync-check`** — for EN docs changed outside `docs/zh/`, `docs/api/`,
  `docs/dev/`, and `docs/spec/`, runs `scripts/check-doc-sync.sh --check`;
  always runs `scripts/check-links.sh` and `scripts/check-brain.sh`.
- **`build-matrix`** — compiles five configurations: `all-on`, `minimal`,
  `router-cache`, `static-files`, and `no-compression`.

Tests intentionally do not run here. Run them locally:

```bash
make build && make test
```

## Release Gate (`ci-release-gate.yml`)

The gate fires on the `release` event with type `published`, which covers both
stable releases and prereleases created by
`gh release create <tag> --prerelease`. Drafts do not trigger it. Runs for tags
starting with `nightly-` are skipped because nightly covers them.

Jobs:

- **`ubuntu-build`** — Release build with examples.
- **`ubuntu-test-fast`** — fast suite through the reusable `run-tests` action.
- **`build-matrix`** — all-on, router-cache, static-files, and no-compression
  run fast tests; the `minimal` configuration is compile-only.
- **`asan-gate`** — Debug + ASan with static files and router cache enabled,
  full suite, serial execution.
- **`generate-summary`** — aggregates results.

The benchmark gate lives in `ci-benchmark.yml` and fires on the same prerelease
event, so a red benchmark blocks promotion to a stable release.

## Nightly and Scheduled Work

`ci-nightly.yml` runs:

- all-on build (`BUILD_WITH_STATIC_FILES=ON`, `BUILD_WITH_ROUTER_CACHE=ON`);
- security scan (CodeQL and dependency checks);
- coverage (`lcov`, report artifact, Codecov upload);
- stress tests;
- UBSan tests;
- an issue is created when the run fails.

`ci-fuzz.yml` runs the libFuzzer harnesses daily. `weekly-retro-check.yml`
enforces the Friday retro artifact described in
[development-rhythm.md](../development-rhythm.md).

## Benchmark Gate (`ci-benchmark.yml`)

The benchmark gate compares head and base on the same runner in the same job,
alternating rounds, and gates on the median of paired ratios. Gate endpoints
are `/`, `/json`, and `/large`; a missing endpoint fails closed.

- Default base: previous published release.
- Manual runs accept `rounds`, `duration`, `endpoints`, and `base_ref`.
- Do not derive performance conclusions from local multi-round runs. See the
  measurement policy in [CONTRIBUTING.md](https://github.com/adam-ikari/uvhttp/blob/main/CONTRIBUTING.md).

## Documentation Deployment (`deploy-docs.yml`)

Triggers on pushes to `main` that touch `docs/**`, `docs/Doxyfile`,
`scripts/docs/convert_xml_to_markdown.js`, or the workflow itself. It runs
Doxygen, converts XML to Markdown, refreshes the doc-sync status, builds
VitePress, and deploys `docs/.vitepress/dist` to GitHub Pages.

Local equivalent:

```bash
make docs          # Doxygen + API Markdown + VitePress build
make check-docs    # doc sync + internal links
```

## Reproducing CI Locally

| CI job | Local command |
|---|---|
| `ubuntu-build` | `make build-release` |
| `code-quality-check` | `cppcheck --enable=warning,performance,portability --error-exitcode=1 src/` |
| `doc-sync-check` | `make check-docs` (also `bash scripts/check-brain.sh`) |
| `build-matrix` | `make build CMAKE_FLAGS="..."` with the matrix flags |
| `ubuntu-test-fast` | `make test` |
| `asan-gate` | `make verify-memory-safety` |
| coverage | `make build-coverage && make coverage` |
| docs deployment | `make docs` |

## Common Failures

| Symptom | Cause and fix |
|---|---|
| `doc-sync-check` red | EN and ZH must be updated in the same commit; ZH must not be older. |
| `minimal` matrix red | Test or source relies on a transitive include that feature trimming removed. Include headers directly. |
| ASan job sees zero tests | Static-file tests are skipped when `BUILD_WITH_STATIC_FILES=OFF`; the gate enables it explicitly. |
| Release gate did not run | A draft release does not fire `published`; publish the prerelease. |
| Benchmark gate fails closed | An endpoint in `GATE_ENDPOINTS` produced no CSV row. |
