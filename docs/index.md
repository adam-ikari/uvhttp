---
layout: home

hero:
  name: UVHTTP
  text: 高性能 HTTP/1.1 和 WebSocket 服务器库
  tagline: 基于 libuv 事件驱动架构，为现代 C 应用提供极致性能
  actions:
    - theme: brand
      text: 快速开始
      link: /guide/getting-started
    - theme: alt
      text: API 文档
      link: /api/introduction
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🚀 极致性能
    details: 峰值吞吐量达 17,798 RPS，基于 libuv 事件驱动，集成 xxHash 极快哈希算法
  - title: ⚡ 零拷贝优化
    details: 大文件使用 sendfile 零拷贝传输，性能提升 50%+，大幅降低 CPU 占用
  - title: 💾 智能缓存
    details: LRU 缓存 + 缓存预热机制，重复请求性能提升 300%+
  - title: 🔒 安全可靠
    details: 缓冲区溢出保护、输入验证、TLS 1.3 支持，零编译警告
  - title: 🧩 模块化设计
    details: 支持静态文件服务、WebSocket、限流等功能，通过编译宏灵活控制
  - title: 📦 轻量级
    details: 纯 C 实现，无外部依赖，易于集成到现有项目
---

## 为什么选择 UVHTTP？

UVHTTP 专为需要高性能、低延迟 HTTP 服务的 C 语言应用而设计。相比传统的 HTTP 服务器库，UVHTTP 提供了更现代的架构和更优的性能表现。

### 适用场景

- **微服务架构** - 构建高性能的 API 网关和服务端点
- **嵌入式开发** - 在资源受限的设备上提供 HTTP 服务
- **实时通信** - WebSocket 支持实现实时双向通信
- **文件服务** - 高效的静态文件传输，支持大文件零拷贝
- **API 服务器** - 快速构建 RESTful API 服务
- **边缘计算** - 在边缘节点提供低延迟的 HTTP 服务

### 核心优势

| 特性         | UVHTTP             | 传统方案            |
| ------------ | ------------------ | ------------------- |
| **事件驱动** | ✅ 基于 libuv      | ❌ 阻塞式 I/O       |
| **零拷贝**   | ✅ sendfile 支持   | ❌ 多次内存拷贝     |
| **智能缓存** | ✅ LRU + 预热      | ❌ 无缓存或简单缓存 |
| **TLS 支持** | ✅ mbedtls TLS 1.3 | ⚠️ 依赖 OpenSSL     |
| **内存管理** | ✅ 统一分配器      | ❌ 混用 malloc/free |
| **编译警告** | ✅ 零警告          | ⚠️ 常有警告         |
| **代码质量** | ✅ 生产就绪        | ⚠️ 需要额外优化     |

## 性能表现

UVHTTP 在标准硬件上经过严格测试，性能表现优异：

- **低并发吞吐量**：17,798 RPS（2 线程 / 10 连接）
- **中等并发吞吐量**：17,209 RPS（4 线程 / 50 连接）
- **高并发吞吐量**：16,623 RPS（8 线程 / 200 连接）
- **平均延迟**：518 μs - 12.20 ms
- **P99 延迟**：1.6 ms - 40.0 ms
- **错误率**：< 0.1%
- **并发连接**：支持数千并发连接

### 性能优化技术

- **零拷贝传输** - 大文件使用 sendfile，避免内核空间与用户空间的数据拷贝
- **智能缓存** - LRU 缓存策略配合缓存预热，显著提升重复请求性能
- **连接复用** - Keep-Alive 连接池，减少 TCP 握手开销
- **异步 I/O** - 基于 libuv 事件驱动，完全非阻塞
- **高效哈希** - 集成 xxHash 算法，路由匹配速度极快

详细性能测试数据和对比分析请查看 [性能基准文档](PERFORMANCE_BENCHMARK.md)。

## 技术栈

UVHTTP 采用现代化的技术栈，确保高性能和可维护性：

- **核心框架**：libuv（异步 I/O）
- **HTTP 解析**：llhttp（高性能 HTTP 解析器）
- **TLS 支持**：mbedtls（轻量级 TLS 库）
- **哈希算法**：xxHash（极快哈希）
- **内存分配**：mimalloc（可选，高性能内存分配器）
- **JSON 支持**：cJSON（可选）
- **构建系统**：CMake 3.10+
- **测试框架**：Google Test

## 快速体验

只需三个步骤即可开始使用：

1. **克隆仓库**

   ```bash
   git clone https://github.com/adam-ikari/uvhttp.git
   cd uvhttp
   ```

2. **编译项目**

   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

3. **运行示例**
   ```bash
   ./dist/bin/helloworld
   ```

访问 `http://localhost:8080` 即可看到运行效果。

详细的使用指南和更多示例请查看 [快速开始文档](/guide/getting-started)。

## 文档导航

### 📚 选择你的文档路径

#### 我是应用开发者
如果你正在使用 UVHTTP 构建应用：
- **[快速开始](/guide/getting-started)** - 5 分钟快速上手
- **[API 参考](/api/introduction)** - 完整的 API 文档（推荐！）
- **[完整教程](/TUTORIAL.md)** - 从基础到高级的教程
- **[统一 API](/guide/unified-api.md)** - 简化的 API 接口
- **[性能优化](/guide/performance.md)** - 优化你的应用性能

#### 我是贡献者/开发者
如果你正在贡献代码或深入了解内部实现：
- **[开发者指南](/DEVELOPER_GUIDE.md)** - 开发者必读
- **[架构设计](ARCHITECTURE.md)** - 深入了解系统架构
- **[测试指南](TESTABILITY_GUIDE.md)** - 测试标准和覆盖率
- **[性能基准](PERFORMANCE_BENCHMARK.md)** - 详细的性能测试数据
- **[贡献指南](/dev/contributing.md)** - 如何贡献代码

### 📖 完整文档索引

- **[使用者文档](/guide/)** - 应用开发者文档
- **[开发者文档](/dev/)** - 贡献者和维护者文档

## 社区与支持

- **[GitHub](https://github.com/adam-ikari/uvhttp)** - 源代码和问题反馈
- **[Issues](https://github.com/adam-ikari/uvhttp/issues)** - 报告问题和功能请求
- **[Discussions](https://github.com/adam-ikari/uvhttp/discussions)** - 社区讨论和技术交流
- **[贡献指南](CONTRIBUTING.md)** - 如何参与项目贡献

## 许可证

UVHTTP 采用 [MIT License](LICENSE) 开源协议，可以自由用于商业和个人项目。

---

**立即开始**：[快速开始](/guide/getting-started) · [API 文档](/api/introduction) · [GitHub](https://github.com/adam-ikari/uvhttp)
