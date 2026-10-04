# UVHTTP 版本发布策略

## 发布节奏

**按需发布**：有可发布内容且所有门禁通过时才发版，不是每周义务。无新
内容时跳过发布，不强制每周一个版本。

## 版本号规则

遵循 SemVer 2.0: `MAJOR.MINOR.PATCH`

| 版本类型 | 触发条件 | 示例 |
|---------|---------|---------|
| **补丁 (PATCH)** | 测试、文档、bug 修复 | 2.5.0 → 2.5.1 |
| **次要 (MINOR)** | 新功能（向后兼容） | 2.5.0 → 2.6.0 |
| **主要 (MAJOR)** | 不兼容 API 变更 | 2.5.0 → 3.0.0 |

## 发布流程

> **分支策略**：所有 main 变更（含发布 commit）必须走 PR 合并，禁止直推 main。

### 阶段一：预发布

1. **全面测试**
   - `make test` (Debug 101/101)
   - `make verify-memory-safety` (ASan + UBSan)
   - `cd docs && npm run docs:build`

2. **版本准备**
   - 更新 `VERSION` 文件
   - 更新 `docs/guide/CHANGELOG.md`
   - 通过 PR 合并到 main（PR-only，禁止直推）
   - 运行 `scripts/ci/release_checklist.sh --pre`：本地可验证项全过才推进

3. **创建预发布 Release**
   - 创建 Git tag: `git tag v2.x.y`
   - 推送 tag: `git push origin v2.x.y`
   - 创建 pre-release: `gh release create v2.x.y --prerelease`
   - **自动触发 ci-benchmark 回归门禁**（同机配对：head vs 上一个 release，10% 阈值）
   - 门禁绿（CI 通过 / gate 通过）才可转正式

### 阶段二：正式发布

4. **确认门禁**
   - 确认 benchmark 回归门禁为绿（PR 标签或 release 事件均触发 gate）
   - gate 以同一 runner 上 head/base 交替测量的比值判定（配对比值中位数 < 90% 且多数配对也在 90% 以下才失败）；绝对 RPS 基线只作为报告信息，不参与判定（GitHub runner 跨 run 方差约 40%，绝对阈值会 gate 到 runner 运气而非代码）

5. **转正式**
   - `gh release edit v2.x.y --latest`（移除 prerelease 标记）
   - main 经 PR 合并触发文档自动部署
   - 运行 `scripts/ci/release_checklist.sh --post`：tag / release latest / 线上 versions.json 全过才算发布完成
   - 确认网站更新

## 发布检查清单

- [ ] 所有测试通过 (101/101)
- [ ] ASan 零发现
- [ ] UBSan 零发现
- [ ] 文档构建通过
- [ ] CHANGELOG 已更新
- [ ] VERSION 已更新
- [ ] Git tag 已创建并推送
- [ ] Benchmark 回归门禁通过（CI / gate 绿）
- [ ] 预发布 Release 已创建（--prerelease）
- [ ] 正式 Release 已确认（--latest）
- [ ] 网站已部署

## 版本历史

| 版本 | 日期 | 主要内容 |
|------|------|---------|
| v2.9.2 | 2026-10-02 | 行为测试补全 11 组 132 个测试（TLS 证书 / URL 编码 / 条件请求 / gzip 与 LRU 缓存边界 / 响应头扩容 / 跨读续写 / query 参数 / 路由 trie 上限 / 连接超时 / accept 计数配对，均经变异验证）、库版本号改为从 VERSION 文件注入、修一个永远不可能失败的测试、删 format-check 门禁与 ci-daily、ROADMAP 精简 |
| v2.9.1 | 2026-10-01 | CI 门禁修复（format-check 空转 + clang-format 版本钉住 + trend 改推分支开 PR）、CI 权限最小化、死代码清理 |
| v2.9.0 | 2026-10-01 | Fuzz 覆盖扩展（request/websocket/static_path 三 harness）、header 名称 control-char 检查（防响应分割）、integration→manual/ 测试基建清理 |
| v2.8.1 | 2026-09-29 | 同机配对门禁、零拷贝小 body 阈值修复 |
| v2.8.0 | 2026-09-23 | 零拷贝 writev（/large +72.7%）、第二轮评审 17 项修复 |
| v2.7.0 | 2026-08-21 | TLS 会话缓存、CI 性能基准、Platinum tier |
| v2.6.1 | 2026-08-12 | 文档同步、CI 修复与内存安全 |
| v2.6.0 | 2026-08-03 | Health Check、SSE 与可测试性 |
| v2.5.1 | 2026-07-24 | 测试覆盖提升 + 规格完善 |
| v2.5.0 | 2026-03-15 | 版本号更新 |
| v2.4.4 | 2026-02-26 | 发布说明 |
| v2.4.3 | 2026-02-26 | 合并 pre-release：内存泄漏修复 + 文档修复 |
| v2.4.2 | 2026-02-25 | 版本号更新 |
