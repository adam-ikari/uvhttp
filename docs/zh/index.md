---
layout: home

hero:
  name: UVHTTP
  text: 内存安全已验证的 C HTTP 服务器
  tagline: 轻量、可嵌入的 C99 HTTP/1.1 与 WebSocket 库——同类中唯一经 ASan 与 UBSan 验证零内存安全问题者。支持 32 位嵌入式，GitHub CI runner 上约 140K RPS，零泄漏。吞吐可测，内存安全可证。
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
    details: ASan + UBSan 零问题——119/119 测试，零泄漏，CI 持续验证。
  - title: 🚀 稳定吞吐
    details: GitHub CI runner 上约 140K RPS；10 轮测试同一次运行内 CV 0.9–2.6%。
  - title: 📦 轻量且 32 位
    details: 静态库 ~257 KB，支持 32 位，冷启动 ~310ms。
  - title: ⚡ 零拷贝文件
    details: 超过 1MB 的文件走 sendfile，大内存 body 走双 iovec writev。
  - title: 💾 智能缓存
    details: LRU 缓存 + 自动预热，热门内容直出内存。
  - title: 🔒 安全为先
    details: 溢出防护、防响应拆分、TLS 1.2/1.3。
  - title: 🧩 模块化
    details: WebSocket / 静态文件 / TLS / 路由缓存编译期开关。
  - title: 📐 干净 API
    details: 一致命名、统一错误，易学难误用。

---

## 📊 性能基准

### 关键指标（v3.0.0 发布门禁，GitHub CI runner）

以下数据来自 v3.0.0 发布基准：**GitHub Actions `ubuntu-latest`**、Release 构建、系统分配器、10 并发连接、每轮 10 秒、每端点 10 轮。同一次运行内中位数稳定（CV 0.9–2.6%）；同一 commit 跨 run 的绝对值会因共享 runner 漂移数十个百分点，因此绝对 RPS 仅作文档参考。回归门禁在**同一 runner、同一 job 内**配对比较 head 与 base：v3.0.0 相对 v2.9.2 为 `/` 100.8%、`/json` 102.0%、`/large` 101.4%，30 个配对所有轮次均未低于 90% 限制。

| 指标 | 数值 | 说明 |
|--------|-------|-------|
| **吞吐量（`/`）** | 约 139K RPS | 10 连接，HTTP/1.1 |
| **吞吐量（`/json`）** | 约 140K RPS | JSON 端点 |
| **吞吐量（`/large`）** | 约 18K RPS | 约 100 KB body，零拷贝 writev |
| **SSE（`/sse`）** | 约 157K RPS | 服务器发送事件 |
| **流式（`/stream`）** | 约 21K RPS | 流式响应 body |
| **WebSocket** | 约 28K msg/s | 回显，10 连接 × 200 条消息 |
| **平均延迟** | 约 70 µs | `/`，10 连接；P99 约 150 µs |
| **高并发** | 约 117K RPS | 1000 连接；过载时客户端出现读超时 |
| **测试套件** | 119/119 通过 | ASan + UBSan 验证通过 |

### v3.0.0 变化

v3.0.0 是一次刻意破坏兼容的发布：删掉无法兑现契约的 API，而不是留着它继续假装。

- **内置限流已移除。** 每个 server 一个计数器，导致单个客户端超限时其余所有客户端都收到 429。取而代之的是分派前的[请求过滤器](/zh/guide/REQUEST_FILTER_API)：窗口长度、计数维度与存储都由应用决定。
- **所有权写进函数名。** `uvhttp_server_set_router` / `set_context` 改名为 `take_router` / `take_context`，builder 层与 `uvhttp_serve` 一并删除。
- **原子性缺陷修复。** `uvhttp_response_set_body()` 不再先释放旧 body 再分配新 body；LRU 缓存 setter 在写入点校验。
- **压缩响应不再声明过期的 `Content-Length`**，静态文件预热不再泄漏它成功处理的每个文件。

完整迁移清单见 [v3.0.0 发布说明](/releases/v3.0.0-release-notes)，逐 PR 记录见[更新日志](/zh/guide/CHANGELOG)。

### 内存安全与质量亮点

- **AddressSanitizer**：完整 119 项测试套件在启用泄漏检测下通过——零泄漏、零 use-after-free、零缓冲区溢出
- **UndefinedBehaviorSanitizer**：完整套件通过——零未定义行为
- **测试用例**：119 项单元/集成测试，全部通过
- **CI/CD**：ASan 在发布门禁运行；UBSan、覆盖率与压力测试每夜运行。Sanitizer 构建启用 `BUILD_WITH_STATIC_FILES` 与 `BUILD_WITH_ROUTER_CACHE`，静态文件与路由缓存代码真正被插桩
- **一键验证**：`make verify-memory-safety`——参见[内存安全](./MEMORY_SAFETY.md)
- **覆盖率**：每夜测量并作为 CI 产物发布

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

大多数轻量 C HTTP 库只追求峰值 RPS。UVHTTP 优化的核心是**内存安全**——这是生产环境不可妥协的属性。一个能在 10 秒基准测试中存活下来的每连接泄漏或 use-after-free，会在一周内让嵌入式设备 OOM。UVHTTP 是轻量、可嵌入、支持 32 位的 C 库，并在发布门禁与每夜 CI 中以 ASan 与 UBSan 双重验证，证明这类 bug 已根除。

| 库 | 可嵌入 C 库 | 32 位 | ASan-clean（已验证） | UBSan-clean（已验证） |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 119/119，CI | ✅ 119/119，CI |
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
- **[发布记录](/releases/)** - 各版本变更与迁移说明
- **[常见问题](/zh/guide/FAQ)** - 常见问题解答