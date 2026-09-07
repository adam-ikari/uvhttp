---
id: release-v272
title: "发布 v2.7.2"
category: decision
status: active
tags: [release, code-review]
created: "2026-09-07T03:36:05"
updated: "2026-09-07T03:36:16"
---

<!-- compiled_truth -->
# v2.7.2 发布记录

## 发布时间
2026-09-07，VERSION_TYPE=patch，34 项代码评审缺陷修复补丁版（commit a0eae2b + release commit）

## 包含内容
- **13 项 P0/P1 关键缺陷修复（commit a3bc755）**：query string 路由匹配、MAX_PARAMS 栈溢出边界、on_url/on_header_field 跨 chunk 分段累积、migrate_to_trie 失败悬垂指针、WS 非 TLS send 短写截帧、CLOSE 后继续处理帧（RFC 6455）、If-Modified-Since 时区错误（mktime→timegm）、accept 失败 active_connections 下溢永久 503、connection_new 失败路径 UAF、server_free 不排空在途 close 回调、超时路径 WS wrapper 泄漏、TLS send 忙等降限
- **21 项 P2/P3 改进与修复（commit a0eae2b）**：TLS EINTR 重试、If-None-Match weak/多值 ETag、目录列表 TOCTOU、on_header_value 分段累积、keep-alive headers_extra 泄漏、X-Forwarded-For 默认不信任（新增 config trust_proxy_headers 开关）、MIME 双表合并单表、TLS cipher 满排空、死代码清理、listen 参数校验、server_stop 幂等化、If-Modified-Since 支持 3 种 HTTP-date 格式

## 门禁结果
- Release 构建 PASS
- 单元测试 101/101 通过
- 文档构建：本地 10 处既有死链导致 Vitepress 严格模式失败（与本次变更无关的存量问题），不阻塞发布

## 兼容性
- 零破坏性变更：patch release，API 完全向后兼容
- 配置新增 `trust_proxy_headers`（默认关闭，X-Forwarded-For 默认不信任为安全默认收紧）

## 文档
- `docs/guide/CHANGELOG.md`：新增 v2.7.2 条目
- `docs/zh/guide/CHANGELOG.md`：新增 v2.7.2 条目
- `docs/releases/v2.7.2-release-notes.md`：发布说明

## 下一步
- v2.8.x 聚焦性能优化与平台扩展：io_uring 探索（P2）、内存分配优化（P2）、新嵌入者接入（P1）、macOS/FreeBSD 支持（P3）


## Timeline

- time: 2026-09-07T03:36:05
  kind: decision
  summary: "Created this page: 发布 v2.7.2"
  source: "v2.7.2 发布会话"
  affects: [release-v272]

- time: 2026-09-07T03:36:16
  kind: decision
  summary: "v2.7.2 发布：34 项代码评审缺陷修复"
  source: "v2.7.2 发布会话"
  affects: [release-v272]
