---
layout: home

hero:
  name: UVHTTP
  text: 内存安全有 CI 门禁的 C HTTP 库
  tagline: C99 写的 HTTP/1.1 与 WebSocket 库。可嵌入，支持 32 位；119 项测试在 ASan 与 UBSan 下零问题，GitHub CI runner 上约 140K RPS。
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
  - title: 内存安全
    details: ASan 与 UBSan 跑完整 119 项测试，零泄漏、零 use-after-free、零越界；发布门禁每夜复跑。
  - title: 吞吐
    details: GitHub CI runner 上约 140K RPS；同一次运行内 CV 0.9–2.6%。
  - title: 体积小
    details: 静态库约 257 KB，支持 32 位，冷启动约 310 ms。
  - title: 零拷贝
    details: 超过 1 MB 的文件走 sendfile，大 body 走双 iovec writev。
  - title: 缓存
    details: LRU 缓存加自动预热，热门文件直接从内存出。
  - title: TLS
    details: 支持 TLS 1.2/1.3，另有溢出与响应拆分防护。

---

## 性能基准

### 关键指标（v3.0.0 发布门禁，GitHub CI runner）

这些数字来自 v3.0.0 的发布基准：GitHub Actions 的 `ubuntu-latest`、Release 构建、系统分配器、10 个并发连接，每个端点 10 轮、每轮 10 秒。同一次运行内中位数稳定（CV 0.9–2.6%）；跨 run 的绝对值会跟着共享 runner 漂移几十个百分点，所以绝对 RPS 只当参考。回归门禁在同一个 runner、同一个 job 内配对比较 head 与 base：v3.0.0 对 v2.9.2 是 `/` 100.8%、`/json` 102.0%、`/large` 101.4%，30 个配对轮次没有一轮低于 90%。

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

v3.0.0 刻意打破了兼容：兑现不了契约的 API 直接删除。

- 内置限流删了。每个 server 一个计数器，结果是一个客户端超限，所有客户端一起收 429。现在换成分派前的[请求过滤器](/zh/guide/REQUEST_FILTER_API)：窗口长度、计数维度和存储交给应用决定。
- 所有权写进了函数名。`uvhttp_server_set_router` / `set_context` 改名为 `take_router` / `take_context`，builder 层与 `uvhttp_serve` 一起删除。
- `uvhttp_response_set_body()` 不再先释放旧 body 再分配新的；LRU 缓存 setter 在写入点校验。
- 压缩响应不再带过期的 `Content-Length`；静态文件预热不再泄漏它处理过的每个文件。

完整迁移清单见 [v3.0.0 发布说明](/releases/v3.0.0-release-notes)，逐 PR 记录见[更新日志](/zh/guide/CHANGELOG)。

### 内存安全与质量亮点

- AddressSanitizer 跑完整 119 项测试，开着泄漏检测：零泄漏、零 use-after-free、零缓冲区溢出。
- UndefinedBehaviorSanitizer 跑完整套件：零未定义行为。
- 119 项单元与集成测试全部通过。
- ASan 在发布门禁跑；UBSan、覆盖率与压力测试每夜跑。Sanitizer 构建打开 `BUILD_WITH_STATIC_FILES` 与 `BUILD_WITH_ROUTER_CACHE`，静态文件和路由缓存代码真的被插桩。
- `make verify-memory-safety` 一条命令复现，见[内存安全](./MEMORY_SAFETY.md)。
- 覆盖率每夜测量，作为 CI 产物发布。

### 性能优化

连接复用（Keep-Alive）省掉每个请求重新建 TCP 的开销；默认开 `TCP_NODELAY` 和 `TCP_KEEPALIVE`。路由用 O(1) 前缀匹配，分配器可选 mimalloc。库直接调 libuv，中间不隔抽象层。

## 核心原则

### 1. 专注核心功能

UVHTTP 只处理 HTTP/1.1 和 WebSocket 的协议细节，业务逻辑不进来。认证、数据库这些由应用自己决定。

### 2. 零开销抽象

抽象都是编译期宏，生产构建没有运行时成本。库直接调 libuv，中间不隔层。

### 3. 极简工程

代码能少写就少写。依赖自带、结构干净，维护成本才压得住。

### 4. 测试分离

生产代码里没有测试专用代码。测试靠链接器 wrap 和外部 mock，库本身保持干净。

### 5. 零全局变量

状态都放在 libuv 的数据指针里（`loop->data` 或 `server->context`），多实例和单元测试互不污染。

### 6. 错误处理

统一的错误类型带代码、描述和恢复提示。每个可能失败的点都检查并上报。

## 为什么选 UVHTTP（对比其他轻量 C HTTP 库）

多数轻量 C HTTP 库把峰值 RPS 当目标。UVHTTP 先把内存安全做成门槛：10 秒基准里看不出来的每连接泄漏或 use-after-free，放到跑一周的嵌入式设备上就是 OOM。UVHTTP 本身是轻量、可嵌入、支持 32 位的 C 库，发布门禁和每夜 CI 都用 ASan 与 UBSan 跑全套，这类 bug 藏不住。

| 库 | 可嵌入 C 库 | 32 位 | ASan-clean（已验证） | UBSan-clean（已验证） |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 119/119，CI | ✅ 119/119，CI |
| libuv-http | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| microhttpd | ✅ | ⚠️ | ❓ 未公开 | ❓ 未公开 |
| mongoose | ✅ | ✅ | ❓ 未公开 | ❓ 未公开 |
| nginx | ❌（独立进程） | ✅ | ✅（大团队） | ❓ |

> “未公开”表示该项目没有公开的 sanitizer-clean 门禁，所以“无 finding”没法验证。UVHTTP 的可以 `make verify-memory-safety` 一键复现。

## 平台支持

### 当前状态

- Linux：完整支持，也是主平台。
- macOS：计划中，优先级最低，没有排期。
- Windows、FreeBSD、WebAssembly：计划中。

### 架构支持

- x86_64（64 位）：完整支持。
- x86（32 位）：完整支持，带嵌入式优化。
- ARM64：计划在未来版本。

UVHTTP 以 Linux 为主平台，32 位嵌入式能用。跨平台扩展还在路线图上。

## 快速安装

克隆仓库时带上子模块，然后用默认选项构建：

```bash
# 克隆仓库（含子模块）
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 使用默认选项构建
make build

# 运行示例服务器
./build/dist/bin/hello_world
```

安装细节和构建选项见[安装指南](/zh/guide/build)。

## 文档

先看[快速开始](/zh/guide/getting-started)；API 查[API 参考](/zh/api/introduction)，构建看[安装指南](/zh/guide/INSTALL_CMAKE)，性能调优看[性能指南](/zh/guide/performance)，版本变更在[发布记录](/releases/)，常见问题在[常见问题](/zh/guide/FAQ)。
