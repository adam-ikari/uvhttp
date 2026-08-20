# uvhttp WebSocket 代码评审报告

**评审范围**：`src/uvhttp_websocket.c`（1118 行）、调用链 `src/uvhttp_connection.c`、测试 `test/unit/test_websocket_boost_coverage.cpp`
**验证方式**：代码审查 + ASan 实测复现（double-free / 内存泄漏 / 分片重组均已编译运行验证）

---

## 一、严重缺陷（内存安全 / 安全，已实证）

### S1. Double-free（ASan 已证实）
- **位置**：`src/uvhttp_websocket.c:235`（`uvhttp_ws_build_frame` 内 `uvhttp_free(buffer)`）与 `:531`（`uvhttp_ws_send_frame` 内 `uvhttp_free(buffer)`）
- **触发条件**：客户端（`is_server=0`）调用 `uvhttp_ws_send_*`，且 context 的 DRBG 未初始化（context 为 NULL，或 `ws_drbg_initialized=0`）。`build_frame` 的 mask 分支先 free 了调用者传入的 buffer，`send_frame` 的错误分支又 free 一次 → **double-free / UAF**。
- **旁证**：`build_frame` 内 3 个错误分支中只有 mask 分支 free buffer，其它分支不 free——行为不一致，说明该 free 是误加。
- **实测输出**：`ERROR: AddressSanitizer: attempting double-free ... uvhttp_ws_build_frame:235 ... uvhttp_ws_send_frame:531`
- **修复**：删除 `uvhttp_ws_build_frame` 第 235 行的 `uvhttp_free(buffer)`（buffer 由调用者所有）。

### S2. 每次发送泄漏 send buffer（ASan 已证实）
- **位置**：`src/uvhttp_websocket.c` `uvhttp_ws_send_frame`，正常路径（`send`/`mbedtls_ssl_write` 成功后）**从不释放** `buffer`。只有两条错误路径释放。
- **影响**：任何一次成功的 WebSocket 发送都泄漏 `buffer_size` 字节。长连接高频发送即为稳定内存耗尽 DoS。
- **实测输出**：`LeakSanitizer: 500000 byte(s) leaked in 20000 allocation(s)`（20000 次发送，每次 25 字节）。
- **修复**：在 `send`/`ssl` 发送成功、`return UVHTTP_OK` 之前 `uvhttp_free(buffer)`。

### S3. 分片消息重组失效 + CONTINUATION 帧被静默丢弃（已实证）
- **位置**：`src/uvhttp_websocket.c` `uvhttp_ws_process_data`（~820–890 行）
- **问题**：代码只在 `opcode == TEXT || BINARY` 时进入 fragment 路径，**从不处理 `UVHTTP_WS_OPCODE_CONTINUATION`（0x0）**。RFC 6455 §5.4 规定：非最终分片之后必须用 CONTINUATION 帧（含最终帧）传输后续数据。当前实现中 CONTINUATION 帧既不进 TEXT/BINARY 分支、也不进 CLOSE/PING 分支，被整体丢弃。
- **后果**：标准多帧分片消息永远无法完成重组，`on_message` 永不触发；`fragmented_message` 残留直至连接关闭。
- **实测输出**：发送 `TEXT(FIN=0,"Hel")` + `CONTINUATION(FIN=1,"lo")` 后 `msg_called=0`（on_message 未调用）。
- **注意**：现有测试 `test_websocket_boost_coverage.cpp` 已**承认**此缺陷——注释写明 "this implementation only enters the fragment path for TEXT/BINARY opcodes"，并用相同 TEXT/BINARY opcode 的 FIN 帧（非 RFC 合法序列）"完成"分片。这是**测试适配错误实现**的反模式，测试掩盖了缺陷而非暴露它。

### S4. 分片累积无上限 + 整数溢出 + NULL 解引用（内存 DoS）
- **位置**：`src/uvhttp_websocket.c:835–867`
- **问题**：
  1. `config.max_message_size` 在 `uvhttp_ws_connection_create` 中赋值后**从未在 process_data 中使用**。攻击者可连续发送非 FIN 分片，`fragmented_capacity` 每次翻倍、`realloc` 无限增长，直至 OOM（每帧仅受 max_frame_size 限制，但累积不受限制）。
  2. `fragmented_size + header.payload_length`（size_t 加法）与 `fragmented_capacity *= 2` 均可能整数溢出。
  3. `fragmented_capacity = header.payload_length * 2` 可能溢出。
  4. `uvhttp_alloc` / `uvhttp_realloc` 返回值未检查 NULL 即 `memcpy` → OOM 时崩溃。
- **修复**：在累积前检查 `fragmented_size + payload_length <= max_message_size`；对容量翻倍做溢出防护；检查分配返回值。

---

## 二、协议合规性缺陷（RFC 6455）

### M1. RSV 位未校验
- `parse_frame_header` 解析 `rsv1/2/3`，但 `process_data` 从不检查。RFC 6455 §5.2：未协商扩展时 RSV 非零即协议错误，**必须关闭连接**。当前静默接受非法帧。

### M2. 控制帧 payload > 125 未拒绝
- RFC 6455 §5.5：控制帧（PING/PONG/CLOSE）payload 必须 ≤ 125 且不可分片，违反必须关闭。当前 PING 处理（:940–955）不检查长度，直接以原 payload echo pong，超长 ping 会得到超长 pong。

### M3. 64 位长度高位 / 最短编码未校验
- RFC 6455 §5.2：length=127 时 64 位长度最高位必须为 0（≥2^63 无效）；长度编码应使用最短形式。当前 `parse_frame_header` 未校验（虽有 `max_frame_size` 兜底，但属合规性缺失）。

### M4. 握手 Header 匹配大小写敏感
- `uvhttp_ws_verify_handshake_response` / `uvhttp_ws_handshake_server` 用 `strstr("Sec-WebSocket-Accept:")`、`strstr("Sec-WebSocket-Key:")`、`strstr("Upgrade: websocket")` 匹配。HTTP 头名大小写不敏感（RFC 7230 §3.2），合法但大小写不同的握手会被拒绝。`websocket_protocol_detector` 的 Connection 头 `strstr(...,"Upgrade")` 同理。

### M5. 协议错误直接断开、不发 close frame(1002)
- 新增的 unmasked 检查返回 `INVALID_PARAM` → `uvhttp_connection_websocket_close` **直接关闭 TCP**（已确认其实现不发送 close 帧）。RFC 6455 §7.1.7 建议对协议错误发送 1002 close 帧后关闭。当前行为满足 §5.1 的 "MUST close"，但未按最佳实践完成 close 握手。

---

## 三、代码质量问题

### L1. `no_sanitize("address")` 掩盖而非修复
- `uvhttp_ws_close` 用 `__attribute__((no_sanitize("address")))` 规避 ASan。注释声称是 "false positive"，但这样该函数完全脱离 ASan 保护，真实越界将无法被发现。应定位根因，而非禁用 sanitizer。

### L2. 重复注释
- `uvhttp_ws_close`（:582–588）有两段几乎相同的注释块，疑似 merge 冲突残留。

### L3. `build_frame` 返回类型错误
- 声明返回 `uvhttp_error_t`（int enum），实际返回 `total_size`（size_t）。帧 > 2GB 时返回值截断为负 int → `send_frame` 的 `frame_len < 0` 误判。应改为返回 `ssize_t`/`size_t`。

### L4. `recv_frame` 长度转 int 截断
- `ret != (int)frame->header.payload_length` 与 `ret != (int)(header_size-2)`：uint64_t 长度转 int 为实现定义行为，> INT_MAX 时比较可能误判。

---

## 四、当前分支改动（未提交）本身评审

新增的 server 端 unmasked 帧拒绝（`conn->is_server && !header.mask → INVALID_PARAM`）**方向正确**，符合 RFC 6455 §5.1，且测试辅助函数 `build_raw_frame` 相应改为发送 masked 帧，逻辑自洽。两个注意点：
1. 该检查在 `total_frame_size` 是否足够之前返回——只要解析到未 mask 的 2 字节头就立即关闭连接，符合 "MUST close"，可接受。
2. 触发后直接断开而不发 1002 close 帧（见 M5）。

---

## 五、修复优先级建议

| 优先级 | 缺陷 | 修复成本 | 说明 |
|---|---|---|---|
| P0 | S1 double-free | 删 1 行 | 崩溃/UAF，任何客户端在 DRBG 未就绪时发送即触发 |
| P0 | S2 每次发送泄漏 | 加 1 行 free | 稳定内存 DoS，线上必须修 |
| P1 | S3 CONTINUATION 未处理 | 中等 | 分片消息功能完全失效 + 测试需重写为 RFC 合法序列 |
| P1 | S4 分片无上限/溢出 | 中等 | 内存 DoS；需在累积处强制 max_message_size |
| P2 | M1–M5 | 小–中 | 协议合规性 |
| P3 | L1–L4 | 小 | 代码质量 |
