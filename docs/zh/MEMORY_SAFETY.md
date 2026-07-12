# 内存安全

UVHTTP 最突出的差异化优势并非单机峰值吞吐（这方面 nginx 与 h2o 更胜一筹）——而是**在一个轻量、可嵌入、支持 32 位的 C 库中提供经过验证的内存安全**。对于无法容忍长期运行中出现缓慢内存泄漏、或在持续运行数周后出现堆内存释放后使用（use-after-free）的生产服务与嵌入式设备而言，这才是真正关键的属性，也是大多数同类轻量级 C HTTP 库所不具备的。

本文档即为这一主张的工程证据。

## 承诺

完整的 91 项测试套件在以下**两种**配置下均验证通过：

| 消毒器 | 检测内容 | 结果 |
|--------|----------|------|
| **AddressSanitizer**（含内存泄漏检测） | 堆内存释放后使用、堆缓冲区溢出、栈缓冲区溢出、内存泄漏 | **91/91 pass，零发现** |
| **UndefinedBehaviorSanitizer** | 有符号整数溢出、非法移位、空指针解引用、对齐错误、越界访问 | **91/91 pass，零发现** |

ASan 与 UBSan 无法在同一个构建中共存，因此以两个独立配置分别运行。两者必须全部为绿色（通过）。

## 自行复现（一条命令）

```bash
make verify-memory-safety
```

该命令会在 ASan 与 UBSan 下分别配置并构建测试套件，在每种消毒器下运行完整的 91 项测试，若任一测试失败或任一消毒器报告发现，则以非零状态码退出。一次干净的运行会输出：

```
==> PASS: full suite clean under ASan and UBSan (91/91 each).
```

等价的手动命令如下：

```bash
# AddressSanitizer（内存泄漏、堆内存释放后使用、缓冲区溢出）
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
(cd build_asan && ctest --output-on-failure)

# UndefinedBehaviorSanitizer
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc)
(cd build_ubsan && ctest --output-on-failure)
```

消毒器构建保留调试符号（不执行 `-s` strip），因此任何发现都会产生可解析的、源码级别的栈回溯。

## 持续验证

内存安全是一项**不回退的不变量（non-regressing invariant）**，而非一次性的检查：

- **每夜 CI**（`.github/workflows/ci-nightly.yml`）：`test-memory`（含内存泄漏检测的 ASan）与 `test-ubsan` 作业每夜在各自消毒器下运行完整测试套件。
- **构建系统护栏**：消毒器 / Debug 构建从不被 strip，因此 CI 中的发现始终可调试。

## 为达成此承诺而修复的缺陷类别

测试套件最初并不干净。要做到零消毒器发现，需要在生产级库代码中修复真实的内存安全缺陷——这些缺陷能通过常规测试套件，却在消毒器下破坏内存。代表性修复如下（完整列表见[更新日志](../guide/CHANGELOG.md)）：

| 缺陷类别 | 位置 | 详情 |
|----------|------|------|
| 堆内存释放后使用 | `uvhttp_router.c` `find_or_create_child` | 指向节点池的缓存指针在 `realloc` 扩展池后悬空 |
| 堆内存释放后使用 | `uvhttp_server.c` `uvhttp_server_free` | 双重释放时从已释放内存读取 `freed` 标志 |
| 堆内存释放后使用 | `uvhttp_connection.c` | 关闭后读取；`close_pending` 计数在重入时出错 |
| 堆缓冲区溢出 | `uvhttp_response.c` `build_response_headers` | `snprintf` 返回值累加使 `pos` 超出缓冲区，下溢突破边界 |
| 栈缓冲区下溢 | `uvhttp_websocket.c` `uvhttp_ws_close` | 写入已超出作用域的栈上连接 |
| 内存泄漏 | `uvhttp_connection.c` `restart_read` | 请求体指针被置空却未释放 |
| 内存泄漏 | `uvhttp_server.c` `on_connection` 503 路径 | `temp_client`（`uv_tcp_t`）从未关闭 / 释放 |
| 内存泄漏 | `uvhttp_server.c` `ws_disable_connection_management` | 管理结构体（含内嵌 `uv_timer_t`）在关闭回调触发前被释放 |
| 内存泄漏 | `uvhttp_error_helpers.c` | 对非 libuv errno 调用 `uv_strerror` 触发了无法释放的 libuv `uv__strdup` |
| 未定义行为 | `uvhttp_connection.c` `switch_to_websocket` | 重入破坏 CLOSING 状态，导致关闭计数错乱（表现为 UBSan 下的挂起） |

每一项均经过根因分析（而非掩盖），并在源码层面修复。

## 为何重要（以及为何多数轻量级 C 库缺失此项）

一个“通过测试”的 C HTTP 库仍可能在每次连接中泄漏数 KB 内存、在罕见的关闭竞态中解引用已释放内存，或把一段未以 null 结尾的字符串拼进响应。这些都不会令功能测试失败。它们表现为：

- **RSS 缓慢增长**，持续运行数天 / 数周（嵌入式设备无法重启）。
- **间歇性崩溃**，发生在测试套件从未触及的生产负载模式下。
- **可达安全性的内存破坏**（头部 / 主体解析中的缓冲区溢出）。

消毒器正是在部署**之前**发现这些问题的手段。多数轻量级 C HTTP 库之所以不宣传“消毒器干净”的状态，并非因为它们干净——而是因为它们根本没有运行这一检查。UVHTTP 在每个每夜构建中运行它，并将任何发现视为发布阻断项。

## 纵深防御

除消毒器外，代码库还应用了：

- **`-Werror`** 配合 `-Wall -Wextra -Wformat=2 -Wformat-security`：零警告的编译门槛。
- **`-fstack-protector-strong`**、`-fno-common`、完整 RELRO（`-Wl,-z,relro,now`）。
- **HTTP 响应拆分防护**（`contains_control_chars`），对所有头部值在发出前执行检查。
- **输入校验**：URL 长度 / 路径穿越、头部大小 / 格式、主体大小限制。
- **幂等资源清理**：`*_cleanup` / `*_free_resources` 路径在释放后将指针置空，并依据 `freed` 标志做防护，以在 API 边界处对双重释放保持健壮。

## 当报告了消毒器发现时

1. 用 `make verify-memory-safety`（或特定的消毒器构建）复现。
2. 依据源码级栈回溯定位根因（消毒器构建未被 strip）。
3. 在源码层面修复——不要抑制。
4. 合并前确认 ASan 与 UBSan 均为绿色。

与安全相关的发现，请按[安全指南](./SECURITY.md)中的流程报告。
