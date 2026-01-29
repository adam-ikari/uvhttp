# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.2.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Tests](https://img.shields.io/badge/tests-280%20passing-brightgreen.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 可配置 • 生产就绪

</div>

## ✨ 特性

- ⚡ **高性能**: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法，峰值吞吐量达 23,226 RPS
- 🔒 **安全**: 缓冲区溢出保护、输入验证、TLS 1.3 支持
- 🛡️ **生产就绪**: 零编译警告、完整错误处理、性能监控
- 🔧 **易于使用**: 简洁的 API、丰富的示例、完善的文档
- 🔄 **连接管理**: 连接池、超时检测、心跳检测、广播功能
- ⚙️ **可配置**: 36 个编译时配置选项，支持不同场景优化
- 💾 **智能缓存**: LRU 缓存 + 缓存预热，零拷贝大文件传输

## 📊 性能指标

- **峰值吞吐量**: 23,226 RPS（低并发场景）
- **静态文件**: 12,510 RPS（中等并发）
- **API 路由**: 13,950 RPS
- **平均延迟**: 2.92ms - 43.59ms
- **错误率**: < 0.1%
- **内存优化**: 使用 mimalloc 分配器，内存占用减少 88%

详细的性能基准测试请查看 [性能测试报告](docs/dev/PERFORMANCE_BENCHMARK.md)

## 🚀 快速开始

### 编译

```bash
mkdir build && cd build
cmake ..
make
```

### 自定义配置

UVHTTP 支持通过 CMake 自定义配置，例如：

```bash
# 高并发场景：增加最大连接数和 headers 容量
cmake -DUVHTTP_MAX_CONNECTIONS_MAX=10000 -DUVHTTP_INLINE_HEADERS_CAPACITY=64 ..

# 大文件传输：增加 body 大小限制
cmake -DUVHTTP_MAX_BODY_SIZE=10485760 ..

# 内存受限场景：减少缓冲区大小
cmake -DUVHTTP_INITIAL_BUFFER_SIZE=4096 -DUVHTTP_READ_BUFFER_SIZE=8192 ..
```

详细的配置说明请查看 [CMake 配置指南](docs/guide/CMAKE_CONFIGURATION.md)

### 运行示例

```bash
./build/dist/bin/helloworld
```

访问 http://127.0.0.1:8080 查看结果

## 📖 文档

详细文档请查看 [docs/](docs/) 目录：

### 核心文档
- [CMake 配置指南](docs/guide/CMAKE_CONFIGURATION.md) - 编译时配置选项详解
- [API 参考](docs/API_REFERENCE.md) - 完整的 API 文档
- [架构设计](docs/ARCHITECTURE.md) - 系统架构说明
- [开发者指南](docs/DEVELOPER_GUIDE.md) - 开发指南
- [教程](docs/guide/TUTORIAL.md) - 从基础到高级的渐进式教程

### 功能指南
- [WebSocket 指南](docs/guide/websocket.md) - WebSocket 使用指南
- [静态文件服务](docs/guide/STATIC_FILE_SERVER.md) - 静态文件服务指南
- [限流 API](docs/guide/RATE_LIMIT_API.md) - 限流功能 API
- [统一响应处理](docs/guide/UNIFIED_RESPONSE_GUIDE.md) - 响应处理最佳实践
- [libuv 数据指针](docs/guide/LIBUV_DATA_POINTER.md) - 避免全局变量的最佳实践

### 示例程序
- [示例程序总览](examples/README.md) - 所有示例程序的完整列表和说明
- [基础示例](examples/01_basics/) - Hello World 和快速入门
- [路由示例](examples/02_routing/) - URL 路由和 HTTP 方法处理
- [中间件示例](examples/03_middleware/) - 中间件系统使用
- [静态文件示例](examples/04_static_files/) - 静态文件服务
- [WebSocket 示例](examples/05_websocket/) - 实时通信
- [高级示例](examples/06_advanced/) - 高级功能和最佳实践

### 项目网站
完整的项目文档和示例请访问：https://adam-ikari.github.io/uvhttp/

## 🧪 测试

```bash
cd build
ctest
```

### 测试覆盖
- **单元测试**: 34 个测试
- **响应测试**: 34 个测试
- **请求测试**: 59 个测试
- **哈希测试**: 65 个测试
- **验证测试**: 53 个测试
- **TLS 测试**: 33 个测试
- **集成测试**: 2 个测试
- **总计**: 280 个测试全部通过

### 代码覆盖率
运行详细覆盖率测试：
```bash
./run_tests.sh --detailed
```

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

### HTTP 流式传输支持
- [ ] 实现分块传输编码（Chunked Transfer Encoding）
- [ ] 支持大文件流式上传（避免内存溢出）
- [ ] 支持大文件流式下载（边读边发）
- [ ] 添加流式响应 API
- [ ] 支持进度回调
- [ ] 优化流式传输性能
- [ ] 添加流式传输示例