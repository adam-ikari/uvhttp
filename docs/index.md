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
  - title: Extreme Performance
    details: Peak throughput up to 23,226 RPS
  - title: Zero-Copy Optimization
    details: Large files use sendfile zero-copy transmission
  - title: Smart Caching
    details: LRU cache with cache preheating mechanism
  - title: Secure and Reliable
    details: Buffer overflow protection and input validation
  - title: Modular Design
    details: Supports static file serving and WebSocket
  - title: Lightweight
    details: Minimal dependencies and easy to embed
  - title: Easy to Use
    details: Clean API design with comprehensive documentation
  - title: Production Ready
    details: Complete error handling and resource management

---

## Platform Support

Current Support: Linux

Future Plans: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for Linux platforms. We plan to expand support to other operating systems and platforms in future releases.