---
id: docs-site-review-3126
title: "文档与网站评审：3.1.x 对齐与 benchmark 实测"
category: decision
status: active
tags: [docs, website, benchmark, review]
created: "2026-11-27T11:54:48"
updated: "2026-11-27T14:48:20"
---

<!-- compiled_truth -->
# 文档与网站评审（2026-11-27）— 修复后

## 开发者支持缺口已清零

上一轮评估"约 80%，未完全达到生产级"，列出 5 条缺口，本轮全部处理：

1. **check-doc-sync.sh 假阴性**：新增 h2（二级标题）数量比对——时间戳相同但章节数不同的中英文件现在判为 STRUCTURE MISMATCH 并阻塞。变异验证：删一个 h2 → 门禁 exit 1；恢复 → 0。这是最有价值的一条，防的是"全绿但内容不对"。
2. **STATIC_FILE_SERVER 中英不对等**：中文多出的「配置选项 / 文件类型控制 / 性能优化 / 故障排除 / 示例项目 / 许可证 / 另见」补进英文；两侧 h2 均为 14。
3. **index.md 中英不对等**：中文多出的"为什么选 UVHTTP"对比表补进英文。
4. **config-api.md** 旧描述"configuration lives at server level"改为 3.1.x 单通道（context + take_config/get_config）。
5. **zh CHANGELOG frontmatter** "涵盖 1.0.0 至 2.7.2" → 3.1.1；**EMBEDDING_GUIDE** 两份 Next steps 补新示例与 Web 管理台。

## 唯一未改（非文档 bug）

3.1.1 尚未打 tag：站点 downloadUrl 指向真实最新 tag v3.0.1，VERSION=3.1.1。这是发布流程状态，需用户打 tag，不是文档错误。

## 验证

`check-doc-sync --check`：31 对、0 过时、0 缺失、0 结构不符；`make check-docs` 链接全有效；VitePress `npm run docs:build` 成功；四配置 ctest 135/136/135/135 全绿；响应体长度门禁与 test-validity PASS。

## 结论

按"照文档写代码能编译、跑通、出错能诊断、答案找得到"判据，开发者支持现在达到生产级：已删 API 全部清除、spec 与实现一致、中英结构对等且门禁可检出不对等、错误系统 83 码含诊断与建议。


## Timeline

- time: 2026-11-27T11:54:48
  kind: decision
  summary: "Created this page: 文档与网站评审：3.1.x 对齐与 benchmark 实测"
  source: "2026-11-27 用户要求跑 benchmark 并评审文档网站"
  affects: [docs-site-review-3126]

- time: 2026-11-27T12:58:55
  kind: decision
  summary: "用户要求修复开发者支持缺口：check-doc-sync.sh 增加 h2 结构比对（变异验证通过），补齐 STATIC_FILE_SERVER 与 index 中英对等，修 config-api 单通道矛盾、zh CHANGELOG frontmatter、EMBEDDING_GUIDE 新示例链接；3.1.1 未打 tag 属发布动作待用户"
  source: "check-doc-sync.sh + 两份 STATIC_FILE_SERVER.md + config-api.md + EMBEDDING_GUIDE.md"
  affects: [docs-site-review-3126]

- time: 2026-11-27T12:58:55
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-27 开发者支持缺口修复完成：中英结构对等门禁 + 5 条缺口清零"
  affects: [docs-site-review-3126]

- time: 2026-11-27T14:48:20
  kind: reversal
  summary: "上一轮结论'开发者支持达生产级'漏查示例可达性：52 个示例只有 4 个构建目标，README 教 6 个不存在的 make 目标。本轮补齐：Makefile.examples 自动发现 52 个目标并抽取 build_static 的 feature 宏；修复 umbrella header 漏包含 uvhttp_logging.h 导致的 UVHTTP_LOG_* 未定义；清理过时 API 字符串；README 按真实命令重写。52/52 编译通过，四配置 ctest 全绿"
  source: "examples/Makefile.examples + include/uvhttp.h + examples/README.md + 52 个示例编译验证"
  affects: [docs-site-review-3126]
