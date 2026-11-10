---
id: quality-audit-security-hardening
title: "质量审计：header 溢出、目录列表 XSS、流水线与最小配置门禁"
category: decision
status: active
tags: [security, http, static, ci, coverage]
created: "2026-11-10T05:19:04"
updated: "2026-11-10T09:30:45"
---

<!-- compiled_truth -->
2026-11-10 对全库做产品级质量审计（两个只读审计 agent + 本地门禁），落地以下结论：

**安全/正确性**
- `uvhttp_response_set_header` 曾用 `UVHTTP_MAX_HEADER_VALUE_SIZE`(4096) 作为拷贝上限，而 `uvhttp_header_t::value` 只有 2048 字节：2048–4095 字节的值可越过最后一个 inline header。现按 `sizeof` 拷贝并在超长时返回 `INVALID_PARAM`；不再静默截断。
- 目录列表的 `href` 曾直接插入文件名（文本已转义），可被 `x"><svg onload=...>` 或 `javascript:` 文件名利用。现 href 百分号编码（仅保留 RFC 3986 unreserved），文本 HTML 转义。
- HTTP 流水线：同一读窗口的第二个请求会让 `on_message_complete` 在首个响应完成前重入，覆盖/释放仍在 `uv_write` 中的 body。现在 `on_message_begin` 检测到 `parsing_complete` 已置位时返回 `HPE_PAUSED`，置 `response->keepalive=0`，首个响应写完即关闭；剩余字节丢弃。
- `uvhttp_router_cache` 哈希表上限 4096 满后 `find_slot` 返回 `UINT32_MAX` 并越界写；现在返回 `OUT_OF_MEMORY`。
- `uvhttp_server_free` 曾不释放 `ws_connection_manager` 及其两个 timer；现在先调用 `uvhttp_server_ws_disable_connection_management`。
- 连接构造失败路径统一在 `connection_new_close_cb` 释放 `tls_cipher_buf`；HTTPS WebSocket 读路径补上满窗 scratch 字节处理。
- 请求 header 值超过 `UVHTTP_HEADER_VALUE_BUFFER_SIZE` 时拒绝；`.gz` 变体切换后复检 `max_file_size`。

**门禁**
- CMake 只把实际创建的目标注册进 CTest；此前特性裁剪配置会注册未构建的可执行文件，ctest 报 "Not Run" 并按失败计。
- release gate 的 minimal 配置从"只编译"改为真正跑测试（本地 91/91）。
- 覆盖率文档改为实测值（2026-11-10：70.9% 行 / 86.3% 函数），并明确覆盖率只报告、不设门禁。
- 示例程序消除全部 32 条编译告警；fuzz README 的链接命令补齐 libminiz/libxxhash。

验证：Release 120/120、minimal 91/91、ASan+UBSan 119/119、4 个 libFuzzer 各 30s 无崩溃。


## Timeline

- time: 2026-11-10T05:19:04
  kind: decision
  summary: "Created this page: 质量审计：header 溢出、目录列表 XSS、流水线与最小配置门禁"
  source: created via brain create-page
  affects: [quality-audit-security-hardening]

- time: 2026-11-10T05:19:17
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: brain update-truth
  affects: [quality-audit-security-hardening]

- time: 2026-11-10T07:35:21
  kind: decision
  summary: "PR #501 已合并到 main（5e96c38），main 上 Release 120/120 通过"
  source: merge verification
  affects: [quality-audit-security-hardening]

- time: 2026-11-10T09:30:45
  kind: decision
  summary: "静态/缓存审计补充：sendfile 改为单次 open(O_NOFOLLOW)+fstat 消除 stat→open 竞态；根路径不再预先替换成 index_file（否则 index 缺失时目录列表不可达且返回 403，改 404）；static_file_handler_wrapper 现在会使用 fallback_context；LRU put 拒绝超过整体内存预算的条目，不再清空缓存后仍超预算"
  source: fix pass
  affects: [quality-audit-security-hardening]
