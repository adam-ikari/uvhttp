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
    details: ASan + UBSan 零问题——91/91 测试，零泄漏，每夜 CI。
  - title: 🚀 稳定吞吐
    details: ~20K RPS，100→500 连接持平，零 socket 错误。
  - title: 📦 轻量且 32 位
    details: 静态库 ~257 KB，支持 32 位，冷启动 ~310ms。
  - title: ⚡ 零拷贝文件
    details: 原生 sendfile 处理大文件——CPU 降低 50%+。
  - title: 💾 智能缓存
    details: LRU 缓存 + 自动预热，热门内容直出内存。
  - title: 🔒 安全为先
    details: 溢出防护、防响应拆分、TLS 1.3。
  - title: 🧩 模块化
    details: WebSocket / 静态 / TLS / 限流编译期开关。
  - title: 📐 干净 API
    details: 一致命名、统一错误，易学难误用。

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