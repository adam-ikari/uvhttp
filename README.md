# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 生产就绪

</div>

## ✨ 特性

- ⚡ **高性能**: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法
- 🔒 **安全**: 缓冲区溢出保护、输入验证、TLS 1.3 支持
- 🛡️ **生产就绪**: 零编译警告、完整错误处理、性能监控
- 🔧 **易于使用**: 简洁的 API、丰富的示例、完善的文档
- 🔐 **WebSocket 认证**: Token 认证、IP 白名单/黑名单
- 🔄 **连接管理**: 连接池、超时检测、心跳检测、广播功能

## 🚀 快速开始

### 编译

```bash
mkdir build && cd build
cmake ..
make
```

### 运行示例

```bash
./build/dist/bin/helloworld
```

访问 http://127.0.0.1:8080 查看结果

## 📖 文档

详细文档请查看 [docs/](docs/) 目录：

- [API 参考](docs/API_REFERENCE.md) - 完整的 API 文档（4,238 行）
- [架构设计](docs/ARCHITECTURE.md) - 系统架构说明
- [开发者指南](docs/DEVELOPER_GUIDE.md) - 开发指南
- [编码规范](docs/CODING_STYLE.md) - 代码风格规范
- [依赖说明](docs/DEPENDENCIES.md) - 第三方依赖
- [变更日志](docs/CHANGELOG.md) - 版本变更历史

### 项目网站

完整的项目文档和示例请访问：https://adam-ikari.github.io/uvhttp/

## 🧪 测试

```bash
cd build
ctest --output-on-failure
```

### 测试结果

- **快速测试**: 66 个测试（2.16 秒，100% 通过）
- **慢速测试**: 5 个测试（在 nightly build 中运行）
- **代码覆盖率**: 68.6% (行覆盖率), 84.1% (函数覆盖率)

## 📊 性能

### 基准测试结果

**测试条件**: 4 线程，100 并发连接，30 秒测试时长

| 场景 | QPS | 平均延迟 | P99 延迟 |
|------|-----|----------|----------|
| 主页 | 9,769 | 4.87ms | ~10ms |
| 小文件 (< 10KB) | 7,347 | 14.04ms | ~30ms |
| 中等文件 (10KB - 1MB) | 4,444 | 46.70ms | ~100ms |
| 大文件 (> 1MB) | 4,622 | 23.54ms | ~50ms |

### 性能优化

- **连接复用优化**: 避免清零 280KB+ 内存
- **内存布局优化**: 8 字节对齐
- **LRU 缓存**: 优化路由缓存和静态文件缓存
- **零拷贝**: 减少内存拷贝操作

## 📦 安装

### 从源码编译

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
git checkout v2.0.0
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### 依赖要求

- **libuv**: ≥ 1.44.0
- **llhttp**: ≥ 8.0.0
- **mbedtls**: ≥ 3.0.0（可选，用于 TLS 支持）
- **mimalloc**: ≥ 2.0.0（可选，用于内存优化）
- **xxhash**: ≥ 0.8.0

## ⚙️ 配置建议

### 内存配置

**基础内存占用**: < 10MB  
**静态库大小**: 2.4MB  
**每个连接占用**: 约 4-8KB

**内存分配器选择**:

| 场景 | 推荐配置 | 说明 |
|------|----------|------|
| **生产环境** | mimalloc（默认） | 性能最优，碎片少 |
| **嵌入式系统** | 系统分配器 | 内存占用可控 |
| **调试环境** | 系统分配器 | 便于内存分析 |

**内存限制配置**:

```c
uvhttp_config_t* config = uvhttp_config_new();

// 设置最大连接数（影响内存占用）
// 2048 连接 ≈ 16MB 内存
config->max_connections = 2048;

// 设置最大请求体大小（防止内存耗尽攻击）
// 默认: 1MB
config->max_body_size = 10 * 1024 * 1024;  // 10MB

// 设置最大 Header 大小
// 默认: 8KB
config->max_header_size = 16 * 1024;  // 16KB
```

### 连接数配置

**默认配置**: 2048 连接

**配置建议**:

| 场景 | 推荐连接数 | 内存占用 | 说明 |
|------|-----------|---------|------|
| **内部服务** | 500 | ~4MB | 低并发，内部调用 |
| **Web 应用** | 2048 | ~16MB | 中等并发，标准配置 |
| **API 网关** | 5000 | ~40MB | 高并发，流量入口 |
| **微服务** | 10000 | ~80MB | 极高并发，需充足资源 |

### 性能配置

**编译优化**:

```bash
# Release 模式（推荐）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 启用优化选项
cmake -DCMAKE_C_FLAGS="-O3 -march=native -mtune=native" ..

# 禁用调试符号
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -DNDEBUG" ..
```

**运行时配置**:

```c
// 启用 Keep-Alive（默认启用）
config->enable_keepalive = 1;
config->keepalive_timeout = 60;  // 60 秒超时

// 启用 TCP_NODELAY（减少延迟）
config->enable_tcp_nodelay = 1;

// 启用 TCP_KEEPALIVE
config->enable_tcp_keepalive = 1;
config->tcp_keepalive_idle = 30;   // 30 秒空闲
config->tcp_keepalive_interval = 10;  // 10 秒间隔
config->tcp_keepalive_count = 3;    // 3 次重试
```

**性能优化建议**:

1. **使用 mimalloc**: 性能提升 30-50%
2. **启用 Keep-Alive**: 减少连接建立开销
3. **启用 TCP_NODELAY**: 降低延迟
4. **启用零拷贝**: 大文件传输性能提升 50%+

### 安全配置

**TLS 配置**:

```c
// 启用 TLS
config->enable_tls = 1;
config->tls_cert_file = "server.crt";
config->tls_key_file = "server.key";

// TLS 版本（推荐 TLS 1.2+）
config->tls_min_version = 3;  // TLS 1.2

// 验证客户端证书（双向认证）
config->tls_verify_client = 1;
config->tls_ca_file = "ca.crt";
```

**限流配置**:

```c
// 启用限流
config->enable_rate_limit = 1;
config->rate_limit_requests = 1000;  // 每秒 1000 请求
config->rate_limit_window = 60;     // 60 秒窗口
```

**安全建议**:

1. **启用 TLS**: 保护数据传输安全
2. **设置请求大小限制**: 防止内存耗尽攻击
3. **启用限流**: 防止 DDoS 攻击
4. **定期更新依赖**: 保持安全补丁

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📮 联系方式

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues

## 🎯 长期目标

### 云原生支持
- [ ] 支持 WebAssembly 运行时（WASI）
- [ ] 优化边缘计算场景
- [ ] 降低冷启动时间
- [ ] 优化内存占用
- [ ] 支持无服务器架构

### WebAssembly 支持
- [ ] 集成 wasilibuv 替代 libuv
- [ ] 添加 WASM 编译配置
- [ ] 添加 WASI 抽象层
- [ ] 添加 JavaScript 绑定
- [ ] 优化 WASM 内存管理
- [ ] 添加 WASM 性能基准测试

### 边缘计算
- [ ] 优化冷启动时间
- [ ] 降低内存占用
- [ ] 添加离线模式支持
- [ ] 优化网络传输

### 性能优化
- [ ] 缩小与 Nginx 的性能差距
- [ ] 优化中等文件传输性能
- [ ] 添加连接池支持
- [ ] 优化零拷贝传输