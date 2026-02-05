# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.3.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-linux-orange.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 可配置 • 生产就绪

</div>

## 📌 平台支持

**当前支持**: Linux

**未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

## ✨ 特性

- ⚡ **高性能**: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法
- 🔒 **安全**: 缓冲区溢出保护、输入验证、TLS 1.3 支持
- 🛡️ **生产就绪**: 零编译警告、完整错误处理、性能监控
- 🔧 **易于使用**: 简洁的 API、丰富的示例、完善的文档
- 🔄 **连接管理**: 连接池、超时检测、心跳检测、广播功能
- ⚙️ **可配置**: 36 个编译时配置选项，支持不同场景优化
- 💾 **智能缓存**: LRU 缓存 + 缓存预热，零拷贝大文件传输

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

- [API 参考](docs/API_REFERENCE.md) - 完整的 API 文档
- [架构设计](docs/ARCHITECTURE.md) - 系统架构说明
- [贡献者指南](docs/DEVELOPER_GUIDE.md) - 贡献指南和开发规范
- [教程](docs/guide/TUTORIAL.md) - 从基础到高级的渐进式教程
- [示例程序](examples/README.md) - 所有示例程序的完整列表

完整的项目文档和示例请访问：https://adam-ikari.github.io/uvhttp/

## 🧪 测试

```bash
cd build
ctest
```

详细的测试指南请查看 [docs/zh/dev/TESTING_STANDARDS.md](docs/zh/dev/TESTING_STANDARDS.md)

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！详见 [CONTRIBUTING.md](CONTRIBUTING.md)

## 📮 联系方式

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues