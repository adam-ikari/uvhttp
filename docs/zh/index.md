---
layout: home

hero:
  name: UVHTTP
  text: 轻量、可嵌入的 HTTP/1.1 与 WebSocket —— 内存安全已验证
  tagline: 同类 C HTTP 库中唯一经 ASan + UBSan 严格验证内存安全者。支持 32 位嵌入式、零拷贝、~20K RPS——吞吐可测，内存安全可证。
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
  - title: 🛡️ 内存安全已验证（核心差异化）
    details: 全部 91 项测试通过 ASan（零泄漏/UAF/溢出）与 UBSan 验证，每夜 CI 持续验证，`make verify-memory-safety` 一键复现——大多数轻量 C HTTP 库欠缺、却对长期运行与嵌入式至关重要的属性。
  - title: 🚀 稳定扎实的吞吐
    details: ~20K RPS，100 至 500 连接吞吐持平，1M+ 请求零 socket 错误。不追求单机峰值（nginx/h2o 的强项），追求可预测、无泄漏、长寿命的行为。
  - title: ⚡ 零拷贝优化
    details: 大文件使用 sendfile 零拷贝传输，性能提升 50%+，大幅降低 CPU 占用
  - title: 💾 智能缓存
    details: LRU 缓存 + 缓存预热机制，重复请求性能提升 300%+
  - title: 🔒 安全可靠
    details: 缓冲区溢出保护、输入验证、TLS 1.3 支持，零编译警告
  - title: 🧩 模块化设计
    details: 支持静态文件服务、WebSocket、限流等功能，通过编译宏灵活控制
  - title: 📦 轻量级
    details: 最小依赖，易于嵌入，完美适配云原生和微服务架构
  - title: 🔧 易于使用
    details: 清晰的 API 设计，完善的文档，丰富的示例，快速上手
  - title: 生产就绪
    details: 完整的错误处理、资源管理、可观测性、稳定性保证；ASan/UBSan 验证通过

---

## 为什么选 UVHTTP（对比其他轻量 C HTTP 库）

UVHTTP 不追求单机吞吐之冠——那是 nginx/h2o 的强项。其差异化在于：**在轻量、
可嵌入、支持 32 位的 C 库中，提供经过 sanitizer 严格验证的内存安全**——这是长期
运行服务与嵌入式设备最关切的属性，也是大多数同类库所欠缺的。

| 库 | 可嵌入 C 库 | 32 位 | ASan-clean（已验证） | UBSan-clean（已验证） |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 91/91，每夜 CI | ✅ 91/91，每夜 CI |
| libuv-http | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| microhttpd | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| mongoose | ✅ | ✅ | ❓ 未公开 | ❓ 未公开 |
| nginx | ❌（独立进程） | ✅ | ✅（大团队） | ❓ |

> “未公开”表示该项目未发布 sanitizer-clean 测试门禁；无 finding 不可验证。
> UVHTTP 的可通过 `make verify-memory-safety` 一键复现。

## 平台支持

当前支持: Linux

未来计划: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

UVHTTP 目前针对 Linux 平台进行了优化。我们计划在未来版本中扩展对其他操作系统和平台的支持。