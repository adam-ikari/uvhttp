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
    details: ASan + UBSan 零问题——101/101 测试，零泄漏，每夜 CI。
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

## 📊 性能基准

### 关键指标 (v2.6.0)

吞吐量因硬件而异。以下数值具有代表性；在同类 VM 上，该库可维持约 17K–20K RPS（100 连接）/ 约 20K 峰值（低并发），且**零 socket 错误**。使用 `wrk -t4 -c100 -d10s` 复现。

| 指标 | 数值 | 说明 |
|--------|-------|-------|
| **峰值吞吐量** | ~20K RPS | 低并发（10 连接），HTTP/1.1 |
| **高并发** | ~17–19K RPS | 100 并发连接 |
| **静态文件** | 12,510 RPS | 中等并发，1MB 文件 |
| **API 路由** | 13,950 RPS | REST 端点 |
| **平均延迟** | ~9–21 ms | P50–P90，100 连接 |
| **错误率** | 0% | 负载下零 socket 错误 |
| **测试套件** | 101/101 通过 | ASan + UBSan 验证通过 |

### 内存安全与质量亮点

- **AddressSanitizer**：完整 101 项测试套件在启用泄漏检测下通过——零泄漏、零 use-after-free、零缓冲区溢出
- **UndefinedBehaviorSanitizer**：完整套件通过——零未定义行为
- **测试用例**：101 项单元/集成测试，全部通过
- **CI/CD**：每夜 ASan + UBSan 任务（见 `.github/workflows/ci-nightly.yml`）
- **一键验证**：`make verify-memory-safety`——参见[内存安全](./MEMORY_SAFETY.md)

### 性能优化

- **Keep-Alive**：连接复用避免每请求重新建立 TCP
- **TCP**：默认启用 `TCP_NODELAY` 和 `TCP_KEEPALIVE`
- **路由**：O(1) 前缀匹配路由解析
- **分配器**：可选 mimalloc
- **libuv**：直接调用，无抽象层

---

## 🎯 核心原则

### 1. 专注核心功能
UVHTTP 处理 HTTP/1.1 和 WebSocket 协议细节，不强加业务逻辑。应用层控制认证、数据库等特性。

### 2. 零开销抽象
抽象均为编译期宏，生产构建无运行时成本。库直接调用 libuv，无中间层。

### 3. 极简工程
代码库崇尚简洁。自包含依赖和干净架构保持维护成本低。

### 4. 测试分离
生产代码不含测试专用代码。测试使用链接器包装和外部 mock 框架，库保持干净。

### 5. 零全局变量
所有状态保存在 libuv 数据指针（`loop->data` 或 `server->context`）中。支持多实例和单元测试，无全局状态污染。

### 6. 错误处理
统一的错误类型携带代码、描述和恢复提示。每个失败点都被检查和报告。

---

## 为什么选 UVHTTP（对比其他轻量 C HTTP 库）

大多数轻量 C HTTP 库只追求峰值 RPS。UVHTTP 优化的核心是**内存安全**——这是生产环境不可妥协的属性。一个能在 10 秒基准测试中存活下来的每连接泄漏或 use-after-free，会在一周内让嵌入式设备 OOM。UVHTTP 是轻量、可嵌入、支持 32 位的 C 库，并在每夜 CI 中以 ASan 与 UBSan 双重验证，证明这类 bug 已根除。

| 库 | 可嵌入 C 库 | 32 位 | ASan-clean（已验证） | UBSan-clean（已验证） |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 101/101，每夜 CI | ✅ 101/101，每夜 CI |
| libuv-http | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| microhttpd | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| mongoose | ✅ | ✅ | ❓ 未公开 | ❓ 未公开 |
| nginx | ❌（独立进程） | ✅ | ✅（大团队） | ❓ |

> “未公开”表示该项目未发布 sanitizer-clean 测试门禁，故“无 finding”不可验证。
> UVHTTP 的可通过 `make verify-memory-safety` 一键复现。

## 🔧 快速安装

```bash
# 克隆仓库（含子模块）
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 使用默认选项构建
make build

# 运行示例服务器
./build/dist/bin/hello_world
```

详细的安装说明和构建选项请参见[安装指南](/zh/guide/build)。

---

## 📚 文档

- **[快速开始](/zh/guide/getting-started)** - 入门和快速上手
- **[API 参考](/zh/api/introduction)** - 完整的 API 文档
- **[安装指南](/zh/guide/INSTALL_CMAKE)** - 安装和构建指南
- **[性能指南](/zh/guide/performance)** - 性能优化建议
- **[常见问题](/zh/guide/FAQ)** - 常见问题解答