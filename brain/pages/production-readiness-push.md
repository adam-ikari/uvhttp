---
id: production-readiness-push
title: "生产级可用推进：四方向并行，分slice验证"
category: decision
status: active
tags: [production, roadmap, audit, api, ops]
created: "2026-11-24T14:05:24"
updated: "2026-11-27T10:33:46"
---

<!-- compiled_truth -->
# 生产级可用推进（2026-11-26）— 内部审计全部清空

## 最后一项已修：非 WS 协议升级残留字节丢失

自定义协议的升级请求与首帧在同一 TCP 写中到达时，字节已被读入 `conn->read_buffer`，但 `on_read` 无条件清零，新 owner 只拿到 fd，首帧丢失。

修复：扩展 `uvhttp_connection_ownership_callback_t` 签名，新增 `const char* buffered, size_t buffered_len`。`transfer_ownership` 在 read_buffer 中定位 `\r\n\r\n`，把其后的字节作为借用缓冲传给回调（文档注明回调返回后需自行拷贝）。只保留一个 ownership 入口，符合奥卡姆——未新增第二个 API。

同步更新：2 个示例回调、2 个测试 mock/签名，新增 `test_upgrade_buffered.cpp` 两例（有残留/无残留），变异「传 NULL/0」→ 红。中英文 CHANGELOG 的 3.1.0 `Removed (breaking)` 段新增签名变更条目。

## 内部审计四项全部清空

栈 BIO、Upgrade token、TLS WANT_WRITE、非 WS 升级残留字节——均已修复并有变异验证。

## 验证

五配置：Release 134/134、Static 135/135、ASan 134/134、UBSan 134/134、32-bit 134/134；`make check-docs`、`check_test_validity --strict`、cppcheck 通过。

## 剩余

无代码级已知缺陷。外部事项：benchmark 同机配对门禁需 GitHub Actions；下游 embedder 跨实例验证需真实宿主。


## Timeline

- time: 2026-11-24T14:05:24
  kind: decision
  summary: "Created this page: 生产级可用推进：四方向并行，分slice验证"
  source: "2026-11-24 用户拍板：四方向全部做"
  affects: [production-readiness-push]

- time: 2026-11-24T14:19:13
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 slice 1-3 完成，审计结果并入范围"
  affects: [production-readiness-push]

- time: 2026-11-24T14:45:56
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 全四方向交付完成：全门禁 123/123"
  affects: [production-readiness-push]

- time: 2026-11-24T15:15:31
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 续：sendfile keep-alive 交错与静态 HEAD 修复，四配置 124/124"
  affects: [production-readiness-push]

- time: 2026-11-24T15:28:55
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 续2：WS 握手合并帧修复，四配置 125/125"
  affects: [production-readiness-push]

- time: 2026-11-24T15:39:47
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 终：全部四方向交付，四配置 126/126"
  affects: [production-readiness-push]

- time: 2026-11-24T16:04:49
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 终2：P2 WS 两项 + 错误观测器，四配置 128/128"
  affects: [production-readiness-push]

- time: 2026-11-24T16:31:49
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 终3：跨线程 async stop，四配置 128/128，含一次 ASan 泄漏发现与修正"
  affects: [production-readiness-push]

- time: 2026-11-24T17:12:46
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 专项：API 原子化 N3 完成，四配置 128/128"
  affects: [production-readiness-push]

- time: 2026-11-25T09:54:57
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 全部续：日志 sink、排水标志、O5、原子多 header，四配置 130/130"
  affects: [production-readiness-push]

- time: 2026-11-25T11:34:38
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 代码评审修复：按 PHILOSOPHY 逐条收敛，四配置 128/128"
  affects: [production-readiness-push]

- time: 2026-11-25T13:27:17
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 真实 embedder 验证：production_app 示例跑通并 ASan 干净"
  affects: [production-readiness-push]

- time: 2026-11-25T14:28:04
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 续：停止时关闭已接收连接 + 请求体超限回 413，四配置全绿"
  affects: [production-readiness-push]

- time: 2026-11-25T18:07:40
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 全三项完成：背压 + O2 命名统一 + 策略 B，四配置全绿"
  affects: [production-readiness-push]

- time: 2026-11-25T20:57:56
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 发布门槛复核：CI 等价门禁全过，32-bit 仅 vendored miniz 阻断"
  affects: [production-readiness-push]

- time: 2026-11-25T22:02:05
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 32-bit 完整支持：构建矩阵与测试套件端到端通过"
  affects: [production-readiness-push]

- time: 2026-11-26T08:23:06
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 32-bit 升级为 CI 阻塞门禁"
  affects: [production-readiness-push]

- time: 2026-11-26T08:32:32
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 32-bit 门禁覆盖 PR + Pre-Release 双工作流"
  affects: [production-readiness-push]

- time: 2026-11-26T10:31:13
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 两轮审计修复：内存安全、连接生命周期、idle timeout"
  affects: [production-readiness-push]

- time: 2026-11-26T11:10:51
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 CVE 自查 + 三项内部审计修复，全门禁 132/132"
  affects: [production-readiness-push]

- time: 2026-11-26T12:16:42
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 奥卡姆剃刀：删重复机制，压掉 brain 重复正文"
  affects: [production-readiness-push]

- time: 2026-11-26T12:30:04
  kind: decision
  summary: "奥卡姆收敛：删静态 token helper / 超时三副本 / config 双读 / 摘链两副本，修正 backpressure 注释；保留用户明确要求的日志接口与双层守卫"
  source: "只读审视代理 + 五配置 ctest 全绿 + brain lint-links"
  affects: [production-readiness-push]

- time: 2026-11-26T13:04:47
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 奥卡姆收敛第二轮：合并 sendfile 两分支，删两个重复测试，保留公共 ABI 字段"
  affects: [production-readiness-push]

- time: 2026-11-26T13:04:47
  kind: decision
  summary: "奥卡姆第二轮：合并 sendfile medium/large 分支删 ~6.7K 重复，删两个被既有用例覆盖的测试；keepalive 影子字段因公共 ABI 保留待 MAJOR"
  source: "只读比对 + 四配置 ctest 全绿"
  affects: [production-readiness-push]

- time: 2026-11-26T13:33:55
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 用户授权删除 conn->keepalive 死字段，五配置全绿"
  affects: [production-readiness-push]

- time: 2026-11-26T13:33:55
  kind: decision
  summary: "用户授权删除 conn->keepalive 死字段：补 padding 保持尺寸，删 6 处写入与 23 处测试读写，五配置全绿"
  source: "git diff + sizeof 探针 + 五配置 ctest"
  affects: [production-readiness-push]

- time: 2026-11-26T15:44:58
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 TLS WANT_WRITE 挂起修复，五配置全绿"
  affects: [production-readiness-push]

- time: 2026-11-26T16:26:52
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 非 WS 升级残留字节修复，内部审计四项全部清空，五配置全绿"
  affects: [production-readiness-push]

- time: 2026-11-26T17:40:01
  kind: evidence
  summary: "用户澄清 3.x.x 尚未完工，保持 VERSION 3.1.1 不合并；补多 server 实例隔离测试（PHILOSOPHY §10），五配置 135/135 全绿"
  source: "test_multi_instance.cpp + 五配置 ctest"
  affects: [production-readiness-push]

- time: 2026-11-26T20:16:14
  kind: decision
  summary: "用户选择 Web 管理台形态（fetch 真实后端）；新增 examples/10_embedding/www/dashboard.html，同源静态提供。实测发现并修复示例 4 处strlen长度错误（/json 尾随 NUL 导致 JSON.parse 失败）"
  source: "dashboard.html + production_app.c + curl/xxd/JSON.parse 实测"
  affects: [production-readiness-push]

- time: 2026-11-27T08:53:36
  kind: decision
  summary: "用户要求端口由命令行参数指定：main 去掉硬编码默认端口，缺参或非法端口直接 exit 2；实测无参/99999 被拒，18081 端口全端点正常，SIGTERM 排水退出"
  source: "production_app.c main + 实测 curl 与退出码"
  affects: [production-readiness-push]

- time: 2026-11-27T10:19:35
  kind: evidence
  summary: "精确扫描所有 examples 的 uvhttp_respond* body 字面量长度，发现并修 5 处真错（中文 UTF-8 字节数、英文手数错误），全部改 strlen；扫描归零，四个示例单独编译通过"
  source: "Python 括号平衡扫描 + cc 单独编译 + ctest"
  affects: [production-readiness-push]

- time: 2026-11-27T10:33:46
  kind: decision
  summary: "用户授权决定：把响应体长度扫描固化为 CI 门禁（scripts/ci/check_response_lengths.py），接入 PR 与 pre-release 双 job；变异验证「15→5」确认 exit 1；CONTRIBUTING 写入字节数判据"
  source: "check_response_lengths.py + ci-pr.yml + ci-release-gate.yml + CONTRIBUTING.md"
  affects: [production-readiness-push]
