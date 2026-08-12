# TLS 版本声明修正（最小修正·范围表述）设计

## 背景

验证结论（2026-08-12，可复现）：默认服务器配置下，TLS 1.3 与 TLS 1.2 握手均成功（openssl s_client + curl 实测），TLS 1.1 被正确拒绝（no protocols available）。mbedtls 3.6.5 默认最低版本为 TLS 1.2（`MBEDTLS_SSL_MINOR_VERSION_3`），最高为 TLS 1.3（`MBEDTLS_SSL_MINOR_VERSION_4`）；mbedtls 3.x 已彻底移除 TLS 1.0/1.1 支持，无 `MBEDTLS_SSL_MINOR_VERSION_1` 常量。

运行时已同时支持 TLS 1.2/1.3，但以下声明失真，需修正为与实际一致：

- `src/uvhttp_version.c:183` 硬编码 `info->tls_version = "TLS 1.3"`，未反映 1.2–1.3 范围
- README、index、introduction、SECURITY、ROADMAP 多处以 "TLS 1.3" 作为唯一卖点表述

## 范围

不改 API、不加公共接口、不改 mbedtls 配置逻辑。仅修正 1 处代码字符串 + 6 对 EN/ZH 文档表述。

## 改动清单

### 代码（1 行）

`src/uvhttp_version.c:183`：

```c
info->tls_version = "TLS 1.2 - 1.3";
```

字段语义：`uvhttp_get_config_info()` 返回的静态配置信息，描述库支持的协议范围（非当前连接所用版本）。测试（`test/unit/test_version_full_coverage.cpp:447-448,596-597`）仅断言非空，改字符串无破坏。

### 文档（6 对，EN/ZH 对称）

| 文件 | 现状 | 改为 |
|---|---|---|
| `README.md:69` / `README_CN.md:67` | TLS 1.3 Support | TLS 1.2/1.3 Support |
| `docs/index.md:31` / `docs/zh/index.md:31` | TLS 1.3 | TLS 1.2/1.3 |
| `docs/guide/introduction.md:143` / `docs/zh/guide/introduction.md:137` | TLS 1.3 Support: via mbedtls | TLS 1.2/1.3 Support: via mbedtls |
| `docs/guide/SECURITY.md:263` / `docs/zh/guide/SECURITY.md:263` | TLS 1.3: Enabled by default when TLS is used | TLS 1.2/1.3: both enabled by default, TLS 1.3 as maximum |
| `docs/guide/ROADMAP.md:14` / `docs/zh/guide/ROADMAP.md:14` | TLS 1.3 via mbedtls | TLS 1.2/1.3 via mbedtls |

### 不改（历史或已准确）

- `docs/releases/2.3.0.md`、`docs/guide/CHANGELOG.md`（历史记录）
- `docs/guide/FAQ.md` Q18、`docs/guide/SECURITY.md:168` 示例代码（`enable_tls13` API 用法，准确）
- `docs/spec/tls-api.md`（已准确描述 enable/disable 语义，且 spec/ 在 check-doc-sync 排除范围）
- `docs/dev/CI_CD_DESIGN.md`（基准测试场景描述）
- `docs/zh/dev/ROADMAP.md`、`docs/zh/dev/SECURITY.md`（dev/ 排除范围，已准确）
- `docs/guide/SECURITY.md:3` / `zh:3` frontmatter 描述（特性摘要，非错误声明）

## 验证

1. `make test` — 101/101 保持通过
2. `make check-syntax` — 通过
3. `bash scripts/check-doc-sync.sh --check` — 无 outdated 对
4. 重建后运行版本查询，确认输出 `TLS 1.2 - 1.3`
5. `make verify-memory-safety` — ASan/UBSan clean

## 交付

分支 `fix/tls-version-accuracy`，PR 打向 `main`，merge 后删除分支。不提交 `deps/llhttp` 子模块脏状态。
