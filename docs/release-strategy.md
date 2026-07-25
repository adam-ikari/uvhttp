# UVHTTP 版本发布策略

## 每周节奏

每周 5 个工作日 = 1 个版本。

| 天 | Sprint | 活动 | 工作量 |
|---|--------|------|--------|
| 周一 | Sprint 1 | 开发 | 6-7h AI 开发 |
| 周二 | Sprint 2 | 开发 | 6-7h AI 开发 |
| 周三 | Sprint 3 | 开发 | 6-7h AI 开发 |
| 周四 | Sprint 4 | 开发 | 6-7h AI 开发 |
| 周五 | Sprint 5 | 测试 + 发布 | 全面验证 + 打 tag + 部署 |

Sprint 1-4 纯开发，Sprint 5 只做测试和发布，不引入新代码。

## 版本号规则

遵循 SemVer 2.0: `MAJOR.MINOR.PATCH`

| 版本类型 | 触发条件 | 每周示例 |
|---------|---------|---------|
| **补丁 (PATCH)** | 测试、文档、bug 修复 | 2.5.0 → 2.5.1 |
| **次要 (MINOR)** | 新功能（向后兼容） | 2.5.0 → 2.6.0 |
| **主要 (MAJOR)** | 不兼容 API 变更 | 2.5.0 → 3.0.0 |

每周五根据 Sprint 1-4 的内容决定版本号类型。

## 发布流程

### 周五 (Sprint 5)

1. **全面测试**
   - `make -f GNUmakefile test` (Debug 91/91)
   - `make -f GNUmakefile verify-memory-safety` (ASan + UBSan)
   - `cd docs && npm run docs:build`

2. **版本发布**
   - 更新 `VERSION` 文件
   - 更新 `docs/guide/CHANGELOG.md`
   - 创建 Git tag: `git tag v2.x.y`
   - 推送 tag: `git push origin v2.x.y`

3. **部署**
   - Push 到 main 触发 CI 自动部署
   - 确认网站更新

## 发布检查清单

- [ ] 所有测试通过 (91/91)
- [ ] ASan 零发现
- [ ] UBSan 零发现
- [ ] 文档构建通过
- [ ] CHANGELOG 已更新
- [ ] VERSION 已更新
- [ ] Git tag 已创建并推送
- [ ] 网站已部署

## 版本历史

| 版本 | 日期 | Sprint | 主要内容 |
|------|------|--------|---------|
| v2.5.1 | 2026-07-25 | Sprint 3 | 测试覆盖提升 + 规格完善 |
| ... | ... | ... | ... |