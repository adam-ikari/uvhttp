# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.6.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-linux%20%7C%2032--bit-orange.svg)
![Tests](https://img.shields.io/badge/tests-101%2F101%20passing-success.svg)
[![ASan](https://img.shields.io/badge/ASan-clean-success.svg)](https://github.com/adam-ikari/uvhttp/actions/workflows/ci-nightly.yml)
[![UBSan](https://img.shields.io/badge/UBSan-clean-success.svg)](https://github.com/adam-ikari/uvhttp/actions/workflows/ci-nightly.yml)
![Performance](https://img.shields.io/badge/performance-~20K%20RPS-brightgreen.svg)

**内存安全已验证的 C HTTP/1.1 与 WebSocket 服务器库**

轻量可嵌入 • 32 位支持 • 零拷贝 • ASan/UBSan 验证生产级

</div>

## 🎯 概述

UVHTTP 是一个基于 libuv 的生产级事件驱动 HTTP 服务器库，专为现代 C 应用设计。它在极低资源消耗下提供卓越性能，既适用于高性能服务器，也适用于嵌入式系统。

### 关键指标 (v2.6.0)

吞吐量因硬件而异。以下数值来自原始基准测试主机；在同类 VM 上，该库可维持约 17K–20K RPS（100 连接）/ 约 20K 峰值（低并发），且**零 socket 错误**。使用 `wrk -t4 -c100 -d10s` 复现。

| 指标 | 数值 | 说明 |
|--------|-------|-------|
| **峰值吞吐量** | ~20K RPS | HTTP/1.1，低并发（10 连接） |
| **高并发** | ~17–19K RPS | 100 并发连接 |
| **静态文件** | 12,510 RPS | 1MB 文件，零拷贝 |
| **API 路由** | 13,950 RPS | REST 端点 |
| **平均延迟** | ~9–21 ms | P50–P90，100 连接 |
| **错误率** | 0% | 负载下零 socket 错误 |
| **测试套件** | 101/101 通过 | ASan + UBSan 验证通过 |

## 🌍 平台支持

| 平台 | 状态 | 架构 |
|----------|--------|--------------|
| **Linux** | ✅ 完全支持 | x86_64, x86 (32 位) |
| **macOS** | 🔨 开发中 | x86_64, ARM64 |
| **Windows** | 📋 计划中 | x86_64 |
| **FreeBSD** | 📋 计划中 | x86_64 |
| **WebAssembly** | 📋 计划中 | wasm32, wasm64 |

### 32 位嵌入式系统
UVHTTP 完整支持 32 位架构，针对资源受限环境进行了优化，适用于嵌入式设备和物联网应用。

## ✨ 核心特性

### 性能
- ⚡ **卓越性能**：峰值吞吐量 ~20K RPS，低延迟事件驱动 I/O
- 💾 **零拷贝传输**：原生 sendfile 集成，支持大文件（>1MB）
- 🧠 **智能缓存**：LRU 缓存与自动预热机制
- 🚀 **Keep-Alive 优化**：通过连接重用实现约 1000 倍性能提升

### 架构
- 🔧 **模块化设计**：编译期特性选择，支持 WebSocket、静态文件、限流
- ⚙️ **零开销抽象**：所有抽象均为编译期宏，无运行时成本
- 📐 **事件驱动**：基于 libuv 事件循环的非阻塞 I/O
- 🎯 **直接 API 调用**：应用层与 libuv 之间无中间抽象层

### 安全
- 🔒 **安全优先**：全面的缓冲区溢出防护和输入验证
- 🛡️ **TLS 1.2/1.3 支持**：通过 mbedtls 集成实现加密
- ✅ **内存安全**：完整测试套件（101 项）在 AddressSanitizer（无泄漏、无 use-after-free、无溢出）和 UndefinedBehaviorSanitizer 下验证通过
- 🚨 **资源限制**：可配置的连接数、头部大小和请求体大小限制

### 开发者体验
- 📘 **专业 API**：一致的命名约定和直观设计
- 📝 **全面文档**：详尽的指南、API 参考和示例
- 🔍 **精细错误处理**：统一的错误系统，提供诊断和恢复指导
- 🧪 **零编译警告**：严格的代码质量标准

### 高级特性
- 🔄 **连接管理**：连接池、超时检测、心跳监控
- 📊 **限流**：基于令牌桶算法，支持白名单
- 🌐 **WebSocket**：全双工通信，支持 Ping/Pong
- ⚙️ **高度可配置**：36 个编译期选项，适应不同部署场景
- 🎛️ **内存优化**：可选 mimalloc 实现更快的分配

## 🚀 快速开始

### 简便构建方式

UVHTTP 提供基于 Just 的现代构建系统，零依赖。

#### 方式一：Just（推荐）

现代 Just 命令运行器，性能卓越，简洁易用：

```bash
# 安装 Just
./install_just.sh

# 构建 UVHTTP
just build

# 运行测试
just test

# 完整开发工作流
just dev

# 清理构建产物
just clean
```

**优势：**
- ✅ 零依赖（无需 Python）
- ✅ 超快启动（比 Python 快 100 倍）
- ✅ 单一可执行文件（约 1.5MB）
- ✅ 现代命令运行器，功能丰富
- ✅ 跨平台兼容（Linux、macOS、Windows、嵌入式）
- ✅ 31+ 内置任务，覆盖常见操作

#### 方式二：手动构建

偏好手动配置的用户：

```bash
# 克隆仓库
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 构建（Debug 模式）
make build

# 运行测试
make test
```

### 构建示例

```bash
# 构建 UVHTTP 后，轻松编译示例
cd examples
make -f Makefile.examples

# 运行示例
export LD_LIBRARY_PATH=../build/dist/lib:$LD_LIBRARY_PATH
./bin/simple_server
```

### 前置依赖

- **Just 命令运行器**：构建所需（通过 `./install_just.sh` 自动安装）
- **C 编译器**：GCC 4.8+ 或 Clang 3.4+，支持 C99
- **CMake**：3.10 或更高版本
- **构建工具**：make、git
- **可选**：mimalloc 用于提升内存性能
- **Node.js**（用于 llhttp）：从源码构建 llhttp 所需

### 构建 llhttp

构建 UVHTTP 前，需要先构建 llhttp 库：

```bash
# 方式一：使用 npm（推荐）
cd deps/llhttp
npm install
npm run build

# 方式二：使用 make
cd deps/llhttp
make build/libllhttp.a

# 方式三：使用 Python（如 npm 不可用）
cd deps/llhttp
python3 -m http.server 8080 &
npm install
npm run build
```

**注意**：llhttp 库在首次构建后会被缓存，只需构建一次。

### 构建 llhttp（HTTP 解析器）

UVHTTP 使用 llhttp 作为 HTTP 解析器。编译 UVHTTP 前需要先构建它：

```bash
# 进入 llhttp 目录
cd deps/llhttp

# 方式一：使用 npm（推荐）
npm install
npm run build

# 方式二：使用 make（如 npm 不可用）
make build/libllhttp.a

# 返回项目根目录
cd ../..
```

**注意**：llhttp 仅在首次构建时需要。编译后的库会被缓存供后续构建使用。

### 高级构建选项

```bash
# 启用 mimalloc 分配器
cmake -DBUILD_WITH_MIMALLOC=ON ..

# 使用调试符号构建
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 禁用 WebSocket 支持
cmake -DBUILD_WITH_WEBSOCKET=OFF ..

# 32 位嵌入式构建
cmake -DCMAKE_C_FLAGS="-m32" ..
```

### 自定义配置

高级用户可创建自定义配置文件：

```bash
# 复制用户选项模板
cp cmake/UserOptions.cmake cmake/UserOptions.local.cmake

# 编辑文件自定义构建选项
vim cmake/UserOptions.local.cmake

# 使用自定义配置构建
cmake -DCMAKE_USER_CONFIG=ON ..
```

### 基本用法

```c
#include <uvhttp.h>
#include <uv.h>

// 请求处理函数
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello from UVHTTP v2.6.0!");
    return uvhttp_response_send(res);
}

int main() {
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器和路由器
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    // 添加路由
    uvhttp_router_add_route(router, "/hello", hello_handler);
    
    // 启动服务器
    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "服务器启动失败: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    printf("服务器监听在 http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

**编译和运行**：
```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv
export LD_LIBRARY_PATH=./build/dist/lib:$LD_LIBRARY_PATH
./server
```

### 获取帮助

- **Just 任务列表**：`just --list`（显示全部 31 个可用任务）
- **Just 任务帮助**：`just --show <task>`（显示任务详情）
- **快速开始指南**：参见 [JUSTFILE_GUIDE.md](JUSTFILE_GUIDE.md)
- **示例 Makefile**：`make -f examples/Makefile.examples help`
- **文档**：参见 [docs/guide/getting-started.md](docs/guide/getting-started.md)

## 🏗️ 架构

UVHTTP 采用模块化、事件驱动的架构，兼顾性能与灵活性：

```
┌─────────────────────────────────────────────────────────┐
│                    应用层                                 │
│  ┌───────────────────────────────────────────────────┐  │
│  │  业务逻辑与请求处理函数                              │  │
│  │  - 认证                                            │  │
│  │  - 数据处理                                        │  │
│  │  - 响应生成                                        │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                 UVHTTP 框架层                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │ 路由器   │  │ 中间件   │  │WebSocket │            │  │
│  │ O(1)     │  │ 流水线   │  │ 支持    │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │ 静态文件 │  │ 限流     │  │  TLS    │            │  │
│  │         │  │         │  │ 支持    │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                   libuv 事件循环                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │ I/O 事件 │  │ 定时器   │  │ 信号    │            │  │
│  │         │  │ 事件     │  │ 事件    │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
└─────────────────────────────────────────────────────────┘
```

### 关键设计原则

1. **零全局变量**：所有状态通过 libuv 数据指针管理
2. **零开销抽象**：编译期宏，无运行时成本
3. **模块化设计**：编译期特性选择
4. **直接 libuv 集成**：无中间抽象层
5. **资源安全**：全面的错误处理和内存管理

## 📚 文档

### 用户指南
- **[快速开始](docs/guide/getting-started.md)** - 5 分钟快速入门
- **[API 参考](docs/api/introduction.md)** - 完整 API 文档
- **[构建指南](docs/guide/CMAKE_CONFIGURATION.md)** - 构建系统配置
- **[性能基准](docs/performance.md)** - 性能分析和指标

### 开发者资源
- **[架构设计](docs/dev/ARCHITECTURE.md)** - 系统架构与设计决策
- **[开发者指南](docs/guide/DEVELOPER_GUIDE.md)** - 开发最佳实践
- **[测试标准](docs/dev/TESTING_STANDARDS.md)** - 测试指南与覆盖率
- **[迁移指南](docs/MIGRATION_GUIDE.md)** - 版本升级

### 高级主题
- **[WebSocket 指南](docs/guide/websocket.md)** - 实时通信
- **[静态文件服务器](docs/guide/STATIC_FILE_SERVER.md)** - 文件服务优化
- **[限流 API](docs/guide/RATE_LIMIT_API.md)** - 限流实现
- **[压缩特性](COMPRESSION_FEATURE_REPORT.md)** - 零开销压缩

## 🏗️ 项目结构

```
uvhttp/
├── include/              # 公共 API 头文件（27 个文件）
│   ├── uvhttp.h         # 主头文件
│   ├── uvhttp_*.h       # 模块头文件
│   └── uvhttp_features.h # 特性配置
├── src/                 # 实现（23 个 .c 文件）
│   ├── uvhttp_*.c       # 核心模块
│   └── uvhttp_websocket.c # WebSocket 实现
├── docs/                # 文档
│   ├── api/             # API 文档
│   ├── guide/           # 用户指南
│   └── dev/             # 开发者文档
├── examples/            # 示例程序（按主题分类）
│   ├── 01_basics/       # 基础示例
│   ├── 02_routing/      # 路由示例
│   └── 05_websocket/    # WebSocket 示例
├── test/                # 测试套件
│   ├── unit/            # 单元测试（37 个活跃）
│   └── integration/     # 集成测试
├── benchmark/           # 性能基准测试
├── deps/                # 第三方依赖（子模块）
│   ├── libuv/           # 异步 I/O
│   ├── llhttp/          # HTTP 解析器
│   ├── mbedtls/         # TLS/SSL
│   └── mimalloc/        # 内存分配器
└── CMakeLists.txt       # 构建配置
```

## 🧪 测试与质量保证

### 测试覆盖
- **测试套件**：101 个单元/集成测试，全部通过
- **内存安全**：完整套件在 AddressSanitizer（无泄漏、无 use-after-free、无缓冲区溢出）和 UndefinedBehaviorSanitizer（无未定义行为）下验证通过
- **CI/CD**：多平台自动化测试；每夜 ASan + UBSan 作业
- **代码质量**：零编译警告，严格 lint（`-Werror`）

### 运行测试

```bash
# 运行所有测试
./run_tests.sh

# 运行测试并生成覆盖率报告
./run_tests.sh --detailed

# 运行特定测试
cd build
./uvhttp_unit_tests --gtest_filter=TestSuite.TestName

# 内存安全验证（AddressSanitizer，含泄漏检测）
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
cd build_asan && ctest --output-on-failure

# 未定义行为验证（UBSan）
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc)
cd build_ubsan && ctest --output-on-failure
```

### 性能测试

```bash
# 启动性能测试服务器（内置端点：/simple /json /large ...）
./build/dist/bin/test_performance_e2e 8080
#   或统一基准测试服务器：
./build/dist/bin/benchmark_unified 8080

# 运行 wrk 基准测试
wrk -t4 -c100 -d30s http://localhost:8080/simple

# 运行 Apache Bench
ab -n 10000 -c 100 http://localhost:8080/
```

## 🤝 贡献指南

欢迎贡献！请遵循以下指南：

1. 阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献规范
2. 遵循代码风格：C99 标准，4 空格缩进，K&R 大括号
3. 确保所有测试通过：`./run_tests.sh`
4. 零编译警告：已启用 `-Werror`
5. 为新功能添加测试
6. 为 API 变更更新文档

### 拉取请求流程

1. Fork 仓库
2. 创建特性分支：`git checkout -b feature/amazing-feature`
3. 提交变更：`git commit -m 'feat: Add amazing feature'`
4. 推送到分支：`git push origin feature/amazing-feature`
5. 打开 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

### 许可证摘要

- ✅ 可免费用于商业和个人用途
- ✅ 无需署名（但感谢）
- ✅ 可修改和分发
- ✅ 不提供任何担保

## 🙏 致谢

UVHTTP 基于以下优秀的开源项目构建：

- **[libuv](https://github.com/libuv/libuv)** - 异步 I/O 库
- **[llhttp](https://github.com/nodejs/llhttp)** - HTTP 解析器
- **[mbedtls](https://github.com/Mbed-TLS/mbedtls)** - TLS/SSL 库
- **[mimalloc](https://github.com/microsoft/mimalloc)** - 内存分配器
- **[xxHash](https://github.com/Cyan4973/xxHash)** - 快速哈希算法
- **[Google Test](https://github.com/google/googletest)** - 测试框架

## 📞 支持与社区

### 获取帮助
- **GitHub Issues**: [https://github.com/adam-ikari/uvhttp/issues](https://github.com/adam-ikari/uvhttp/issues)
- **讨论区**: [https://github.com/adam-ikari/uvhttp/discussions](https://github.com/adam-ikari/uvhttp/discussions)
- **文档站**: [https://adam-ikari.github.io/uvhttp](https://adam-ikari.github.io/uvhttp)

### 社区
- 在 [GitHub](https://github.com/adam-ikari/uvhttp) 上给我们 Star
- Fork 并参与贡献
- 分享您使用 UVHTTP 的项目
- 报告 Bug 和提出新特性

## 🗺️ 路线图

### v2.6.0（计划中）
- [ ] macOS 平台支持
- [ ] 增强 WebSocket API
- [ ] HTTP/2 支持调研
- [ ] 性能分析工具

### v2.7.0（未来规划）
- [ ] Windows 平台支持
- [ ] gRPC 集成
- [ ] 高级压缩算法
- [ ] Kubernetes 部署指南

## 📊 版本历史

| 版本 | 日期 | 亮点 |
|---------|------|------------|
| **v2.6.0** | 2026-07-31 | 健康检查端点、SSE 示例、mock 测试基础设施、Makefile 构建入口 |
| **v2.5.1** | 2026-07-27 | 覆盖率 86% 行 / 99% 函数，101/101 测试 |
| **v2.5.0** | 2026-03-15 | 32 位嵌入式支持，压缩特性 |
| **v2.4.4** | 2026-01-28 | 性能优化，代码清理 |
| **v2.3.0** | 2026-02-10 | 连接清理性能修复 |
| **v2.2.0** | 2026-01-27 | 重大重构，零开销抽象 |

详见 [CHANGELOG.md](docs/CHANGELOG.md) 发布说明。

---

<div align="center">

**为高性能应用而生**

[⬆ 返回顶部](#uvhttp)

</div>