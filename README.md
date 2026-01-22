# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 生产就绪

</div>

## ✨ 特性

- ⚡ 高性能：基于 libuv 事件驱动架构
- 🔒 安全：缓冲区溢出保护、输入验证、TLS 支持
- 🛡️ 生产就绪：零编译警告、完整错误处理
- 🔧 易于使用：简洁的 API、丰富的示例
- 🔐 WebSocket 支持：认证、连接管理、广播

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

完整的项目文档请访问：https://adam-ikari.github.io/uvhttp/

- [快速开始](https://adam-ikari.github.io/uvhttp/guide/getting-started.html) - 编译和运行示例
- [API 参考](https://adam-ikari.github.io/uvhttp/api/) - 完整的 API 文档
- [性能基准](https://adam-ikari.github.io/uvhttp/performance.html) - 性能测试结果
- [构建指南](https://adam-ikari.github.io/uvhttp/guide/build.html) - 安装和依赖
- [配置指南](https://adam-ikari.github.io/uvhttp/guide/config.html) - 服务器配置
- [测试文档](https://adam-ikari.github.io/uvhttp/guide/testing.html) - 运行测试

## 🧪 测试

```bash
cd build
ctest --output-on-failure
```

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📮 联系方式

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues
