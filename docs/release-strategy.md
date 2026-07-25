# UVHTTP 版本发布策略

## 分支模型

```
main ──────●───────────────●───────────────
            \             /               \
pre-release  ●──●──●────●                 ●──●──●──●
                         \               /
release                   ●─────────────●
```

| 分支 | 用途 | 保护 |
|------|------|------|
| `main` | 日常开发，所有 PR 合入此分支 | ✅ 受保护 |
| `pre-release` | 发布候选，从 main 同步 | ✅ 受保护 |
| `release` | 生产就绪版本，从 pre-release 同步 | ✅ 受保护 |

## 发布流程

### 周五 (Sprint 5)

1. **全面测试**
   - `make -f GNUmakefile test` (Debug 93/93)
   - `make -f GNUmakefile verify-memory-safety` (ASan + UBSan)
   - `cd docs && npm run docs:build`

2. **版本发布**
   - 更新 `VERSION` 文件
   - 更新 `docs/guide/CHANGELOG.md`
   - 创建 Git tag: `git tag v2.x.y`
   - 推送 tag: `git push origin v2.x.y`

3. **同步到 pre-release**
   - 创建 PR: `pre-release` ← `main`
   - 合并后创建 PR: `release` ← `pre-release`

4. **部署**
   - Push 到 main 触发 CI 自动部署网站
   - 确认网站更新

## 版本号规则

遵循 SemVer 2.0: `MAJOR.MINOR.PATCH`

| 版本类型 | 触发条件 | 示例 |
|---------|---------|------|
| **补丁 (PATCH)** | 测试、文档、bug 修复 | 2.5.0 → 2.5.1 |
| **次要 (MINOR)** | 新功能（向后兼容） | 2.5.0 → 2.6.0 |
| **主要 (MAJOR)** | 不兼容 API 变更 | 2.5.0 → 3.0.0 |

## 发布检查清单

- [ ] 所有测试通过 (93/93)
- [ ] ASan 零发现
- [ ] UBSan 零发现
- [ ] 文档构建通过
- [ ] CHANGELOG 已更新
- [ ] VERSION 已更新
- [ ] Git tag 已创建并推送
- [ ] PR `pre-release` ← `main` 已合并
- [ ] PR `release` ← `pre-release` 已合并
- [ ] 网站已部署