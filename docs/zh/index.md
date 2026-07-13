---
layout: home

hero:
  name: UVHTTP
  text: 内存安全已验证的 C HTTP 服务器
  tagline: 轻量、可嵌入的 C99 HTTP/1.1 与 WebSocket 库——同类中唯一经 ASan 与 UBSan 验证零内存安全问题者。支持 32 位嵌入式，~20K RPS，零泄漏。吞吐可测，内存安全可证。
  actions:
    - theme: brand
      text: 快速开始
      link: /zh/guide/getting-started
    - theme: alt
      text: 内存安全
      link: /zh/MEMORY_SAFETY
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🛡️ 内存安全已验证
    details: 全部 91 项测试通过 ASan（零泄漏/UAF/溢出）与 UBSan 验证，每夜 CI 持续验证，`make verify-memory-safety` 一键复现。这是大多数轻量 C HTTP 库欠缺、却对长期运行与嵌入式至关重要的属性。
  - title: 🚀 稳定无泄漏的吞吐
    details: ~20K RPS，100 至 500 连接吞吐持平，1M+ 请求零 socket 错误。不追求单机峰值（那是 nginx/h2o 的强项），追求可预测、无泄漏、长寿命的行为。
  - title: 📦 轻量且支持 32 位嵌入式
    details: 静态库仅 ~257 KB，支持 32 位目标，冷启动 ~310ms。编译期按需启用功能，只为所用付费——完美适配资源受限设备。
  - title: ⚡ 零拷贝文件传输
    details: 原生 sendfile 集成，将大文件（>1MB）直接从内核送到 socket，消除用户态拷贝，CPU 占用降低 50%+。
  - title: 💾 智能缓存
    details: LRU 缓存配合自动预热，热门静态内容从内存直出，减少磁盘 I/O、缩短重复请求响应时间。
  - title: 🔒 安全为先
    details: 每条字符串路径均有缓冲区溢出防护，防响应拆分攻击，严格输入校验，TLS 1.3 基于 mbedtls，`-Werror` 下零编译警告。
  - title: 🧩 模块化架构
    details: WebSocket、静态文件、限流、TLS、压缩均可编译期开关。基于宏的模块化，运行时零开销。
  - title: 📐 专业 API
    details: 一致的命名、直观的请求/响应模型、统一的错误系统（含详细错误码与恢复建议）——易学、难误用。

---

## 为什么选 UVHTTP（对比其他轻量 C HTTP 库）

大多数轻量 C HTTP 库只优化峰值 RPS 便止步。UVHTTP 优化的是**会击垮生产的属性**：内存安全。一个能在 10 秒基准测试中存活下来的每连接泄漏或 use-after-free，会在一周内让嵌入式设备 OOM。UVHTTP 是轻量、可嵌入、支持 32 位的 C 库，并在每夜 CI 中以 ASan 与 UBSan 双重验证，证明这类 bug 已根除。

| 库 | 可嵌入 C 库 | 32 位 | ASan-clean（已验证） | UBSan-clean（已验证） |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 91/91，每夜 CI | ✅ 91/91，每夜 CI |
| libuv-http | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| microhttpd | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| mongoose | ✅ | ✅ | ❓ 未公开 | ❓ 未公开 |
| nginx | ❌（独立进程） | ✅ | ✅（大团队） | ❓ |

> “未公开”表示该项目未发布 sanitizer-clean 测试门禁，故“无 finding”不可验证。
> UVHTTP 的可通过 `make verify-memory-safety` 一键复现。

## 平台支持

当前支持: Linux

未来计划: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

UVHTTP 目前针对 Linux 平台进行了优化。我们计划在未来版本中扩展对其他操作系统和平台的支持。