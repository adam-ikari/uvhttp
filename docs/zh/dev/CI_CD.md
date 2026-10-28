# CI/CD 工作流指南

本文描述当前 `.github/workflows/` 中实际存在的工作流。触发器和 job 以 workflow
文件为准；修改后请同步更新本页。

项目没有 `ci-push.yml`，也没有 `develop` 分支。所有改动通过 PR 合入 `main`，
发布用 tag 与 GitHub prerelease 分阶段进行。

## 工作流总览

| 工作流 | 触发条件 | 作用 |
|---|---|---|
| `ci-pr.yml` | 向 `main` 打开/同步/重新打开 PR | 编译、cppcheck、测试有效性检查、文档同步/链接、配置矩阵。**不跑测试。** |
| `ci-release-gate.yml` | GitHub release 发布（正式或预发布），或手动触发 | 编译、快速测试、带测试的配置矩阵、ASan 门禁。跳过 `nightly-*`。 |
| `ci-benchmark.yml` | 非 nightly 的 release 发布，或手动触发 | 同机配对性能回归门禁。 |
| `ci-nightly.yml` | 每天 00:00 UTC，或手动触发 | all-on 构建、安全扫描、覆盖率、压力、UBSan、失败建 issue。 |
| `ci-fuzz.yml` | 每天 03:17 UTC，或手动触发 | libFuzzer harness。 |
| `weekly-retro-check.yml` | 周五 11:00 UTC，或手动触发 | 检查 `docs/dev/weekly/<YYYY>-W<WW>.md` 是否存在。 |
| `deploy-docs.yml` | push 到 `main` 且涉及文档路径，或手动触发 | Doxygen + API Markdown + VitePress，部署到 GitHub Pages。 |

## PR 门禁（`ci-pr.yml`）

每个 PR 运行四类检查：

- **`ubuntu-build`** — Release 构建，`BUILD_EXAMPLES=ON`，零告警检查，上传产物。
- **`code-quality-check`** — 对 `src/` 跑 cppcheck（`--error-exitcode=1`），并用
  `scripts/ci/check_test_validity.py --strict` 拒绝恒绿测试。
- **`doc-sync-check`** — 当 `docs/zh/`、`docs/api/`、`docs/dev/`、`docs/spec/`
  之外的英文文档有改动时执行 `scripts/check-doc-sync.sh --check`；并始终运行
  `scripts/check-links.sh` 与 `scripts/check-brain.sh`。
- **`build-matrix`** — 编译五种配置：`all-on`、`minimal`、`router-cache`、
  `static-files`、`no-compression`。

测试故意不在 PR 阶段运行。本地执行：

```bash
make build && make test
```

## 发布门禁（`ci-release-gate.yml`）

触发条件是 `release` 事件的 `published` 类型，覆盖正式发布和
`gh release create <tag> --prerelease` 创建的预发布；draft 不会触发。标签以
`nightly-` 开头的运行会被跳过，因为 nightly 已覆盖。

Job：

- **`ubuntu-build`** — Release 构建并编译示例。
- **`ubuntu-test-fast`** — 通过可复用 `run-tests` action 跑快速套件。
- **`build-matrix`** — all-on、router-cache、static-files、no-compression 跑快速
  测试；`minimal` 只验证编译。
- **`asan-gate`** — Debug + ASan，启用 static files 与 router cache，全套串行执行。
- **`generate-summary`** — 汇总结果。

性能门禁在 `ci-benchmark.yml`，由同一预发布事件触发；性能不通过会阻止转正式发布。

## Nightly 与定时任务

`ci-nightly.yml` 运行：

- all-on 构建（`BUILD_WITH_STATIC_FILES=ON`、`BUILD_WITH_ROUTER_CACHE=ON`）；
- 安全扫描（CodeQL 与依赖检查）；
- 覆盖率（lcov，上传报告与 Codecov）；
- 压力测试；
- UBSan 测试；
- 失败时创建 issue。

`ci-fuzz.yml` 每天运行 libFuzzer harness。`weekly-retro-check.yml` 校验
[开发节奏](../../development-rhythm.md) 约定的周五复盘文件。

## 性能门禁（`ci-benchmark.yml`）

门禁在同一个 job、同一 runner 上交替测量 head 与 base，按轮次配对后取比值中位数。
门禁端点为 `/`、`/json`、`/large`；端点缺少 CSV 数据时 fail closed。

- 默认 base：上一个已发布版本。
- 手动触发可传 `rounds`、`duration`、`endpoints`、`base_ref`。
- 不要用本机连续多轮测量下性能结论，规则见
  [CONTRIBUTING.md](https://github.com/adam-ikari/uvhttp/blob/main/CONTRIBUTING.md)。

## 文档部署（`deploy-docs.yml`）

当 push 到 `main` 且改动涉及 `docs/**`、`docs/Doxyfile`、
`scripts/docs/convert_xml_to_markdown.js` 或该 workflow 本身时触发。流程为
Doxygen → XML 转 Markdown → 刷新文档同步状态 → VitePress 构建 → 部署
`docs/.vitepress/dist` 到 GitHub Pages。

本地等价命令：

```bash
make docs          # Doxygen + API Markdown + VitePress 构建
make check-docs    # 文档同步 + 内部链接
```

## 本地复现 CI

| CI job | 本地命令 |
|---|---|
| `ubuntu-build` | `make build-release` |
| `code-quality-check` | `cppcheck --enable=warning,performance,portability --error-exitcode=1 src/` |
| `doc-sync-check` | `make check-docs`（另跑 `bash scripts/check-brain.sh`） |
| `build-matrix` | `make build CMAKE_FLAGS="..."`，传入矩阵 flag |
| `ubuntu-test-fast` | `make test` |
| `asan-gate` | `make verify-memory-safety` |
| 覆盖率 | `make build-coverage && make coverage` |
| 文档部署 | `make docs` |

## 常见失败

| 现象 | 原因与处理 |
|---|---|
| `doc-sync-check` 红 | 中英文必须在同一提交更新，且中文版不能早于英文版。 |
| `minimal` 矩阵失败 | 测试或源码依赖了被裁剪掉的传递 include；改为直接 include。 |
| ASan job 跑到 0 个测试 | `BUILD_WITH_STATIC_FILES=OFF` 会跳过静态文件测试；门禁已显式开启。 |
| 发布门禁没有运行 | draft release 不触发 `published`；请发布 prerelease。 |
| 性能门禁 fail closed | `GATE_ENDPOINTS` 中某个端点没有产生 CSV 行。 |
