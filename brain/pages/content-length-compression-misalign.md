---
id: content-length-compression-misalign
title: "手动 Content-Length + 压缩分帧错位已修复（#464）"
category: decision
status: active
tags: [bug, compression, response, content-length]
created: "2026-10-06T01:19:19"
updated: "2026-10-06T02:39:07"
---

<!-- compiled_truth -->
# 手动 Content-Length + 压缩分帧错位（#464 已修复）

## 缺陷

uvhttp_response_prepare 压缩成功后改写 `response->body_length` 为压缩后长度，
但 build_response_headers 的 gate：

    if (!has_content_length) {
        UVHTTP_SNAPPEND("Content-Length: %zu\r\n", response->body_length);
    }

只在**无手动 Content-Length** 时才用 body_length 生成。若调用方手动设了
Content-Length（静态文件 uvhttp_static.c:659 / 用户路径），它被原样保留
（声明原始长度），而实际发送的是压缩后的 body（更短）→ 客户端按声明长度
读取会卡住或读到后续连接数据（**分帧错位，数据损坏级**）。

## 复现

    body = "AAAA...AAAA" (10000 字节)
    set_header("Content-Length", "10000")
    compress=1, threshold=1024

修复前：声明 CL=10000、实际 body=45（gzip）、错位=是
修复后：声明 CL=45、实际 body=45、错位=否

## 触发面

默认连接 `compress=0`（uvhttp_connection.c:504），不压缩。触发需调用方显式
`compress=1` + 手动 Content-Length——非默认生产路径，但 API 组合下数据损坏。

## 修复（#464）

新增 static helper `uvhttp_response_sync_content_length(response, new_len)`：
压缩成功后遍历 header，把已存在的 Content-Length 值同步为压缩后长度
（build_response_headers 的 gate 保留 header 位置，值正确）。在两处压缩
成功点调用：缓存命中（cached_len）、直接压缩（compressed_len）。未压缩/
压缩无效回退路径不调用——手动 CL 保留原值（一致）。

helper 定义包在 `#if UVHTTP_FEATURE_COMPRESSION` 内——初版在守卫外导致
COMPRESSION=OFF 时 unused-function，build-matrix minimal/no-compression
编译失败（CI 抓到）。

## 测试（4 个，变异验证）

- ManualContentLengthMatchesCompressedBody — 核心：声明 == 压缩后实际 body
- ManualContentLengthPreservedWithoutCompression — 不压缩时手动 CL 保留
- AutoContentLengthMatchesCompressedBody — 无手动 CL 时自动生成正确值
- SmallBodyNoCompressionPreservesManualCL — 小 body 不压缩，CL 保留

变异验证：移除两处 sync 调用 → 核心测试变红（声明 10000 vs 实际 45）。


## Timeline

- time: 2026-10-06T01:19:19
  kind: decision
  summary: "Created this page: 手动 Content-Length + 压缩分帧错位已修复（#464）"
  source: "2026-10-06 修复"
  affects: [content-length-compression-misalign]

- time: 2026-10-06T01:19:19
  kind: decision
  summary: "agent #449 调查发现：压缩成功后 body_length 改为压缩后长度，但 build_response_headers 的 has_content_length gate 只在无手动 Content-Length 时才用 body_length。手动 CL 被保留（声明原始长度）+ 实际发压缩 body → 客户端分帧错位（数据损坏级）。触发面窄（需 compress=1 + 手动 CL，默认 compress=0）。#464 用 uvhttp_response_sync_content_length 在压缩成功后同步 CL header 值。probe 复现：修复前 CL=10000 vs body=45，修复后一致。变异移除调用→测试变红。119/119。"
  affects: [content-length-compression-misalign]

- time: 2026-10-06T02:39:07
  kind: decision
  summary: "手动 Content-Length + 压缩导致客户端分帧错位（数据损坏级），已在 #464 修复"
  source: "#464 修复"
  affects: [content-length-compression-misalign]
