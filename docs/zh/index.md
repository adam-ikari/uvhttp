---
layout: home

hero:
  name: UVHTTP
  text: 高性能 HTTP/1.1 和 WebSocket 服务器库
  tagline: 基于 libuv 事件驱动架构，为现代 C 应用提供极致性能
  actions:
    - theme: brand
      text: 快速开始
      link: /zh/guide/getting-started
    - theme: alt
      text: API 文档
      link: /zh/api/introduction
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🚀 极致性能
    details: 峰值吞吐量约 20K RPS，基于 libuv 事件驱动，集成 xxHash 极快哈希算法，负载下零错误
  - title: 🛡️ 内存安全验证
    details: 全部 91 项测试通过 AddressSanitizer（零泄漏、零 UAF、零溢出）与 UndefinedBehaviorSanitizer（零未定义行为）验证
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

## 平台支持

当前支持: Linux

未来计划: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

UVHTTP 目前针对 Linux 平台进行了优化。我们计划在未来版本中扩展对其他操作系统和平台的支持。