---
layout: home

hero:
  name: UVHTTP
  text: High-performance HTTP/1.1 and WebSocket server library
  tagline: Built on libuv event-driven architecture for ultimate performance in modern C applications
  actions:
    - theme: brand
      text: Quick Start
      link: /guide/getting-started
    - theme: alt
      text: API Docs
      link: /api/introduction
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🚀 Extreme Performance
    details: Peak throughput up to 23,226 RPS, built on libuv event-driven architecture with integrated xxHash ultra-fast hashing algorithm
  - title: ⚡ Zero-Copy Optimization
    details: Large files use sendfile zero-copy transmission, 50%+ performance improvement, significantly reducing CPU usage
  - title: 💾 Smart Caching
    details: LRU cache + cache preheating mechanism, 300%+ performance improvement for repeated requests
  - title: 🔒 Secure & Reliable
    details: Buffer overflow protection, input validation, TLS 1.3 support, zero compilation warnings
  - title: 🧩 Modular Design
    details: Supports static file serving, WebSocket, rate limiting and other features, flexibly controlled via compile-time macros
  - title: 📦 Lightweight
    details: Minimal dependencies, easy to embed, perfect for cloud-native and microservices architectures
  - title: 🔧 Easy to Use
    details: Clean API design, comprehensive documentation, rich examples, quick to get started
  - title: 🧪 Production Ready
    details: Complete error handling, resource management, observability, stability guarantees

## 📌 Platform Support

**Current Support**: Linux (x86_64, x86, ARM64, ARM32)

**Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for Linux platforms. We plan to expand support to other operating systems and platforms in future releases.