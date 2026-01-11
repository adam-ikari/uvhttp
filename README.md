# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.1.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 编译配置 • 生产就绪

</div>

## ✨ 特性

- ⚡ **高性能**: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法
- 🔒 **安全**: 缓冲区溢出保护、输入验证、TLS 1.3 支持
- 🛡️ **生产就绪**: 零编译警告、完整错误处理、性能监控
- 🔧 **易于使用**: 简洁的 API、丰富的示例、完善的文档

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
- [开发者指南](docs/DEVELOPER_GUIDE.md) - 开发指南
- [编码规范](docs/CODING_STYLE.md) - 代码风格规范
- [依赖说明](docs/DEPENDENCIES.md) - 第三方依赖
- [变更日志](docs/CHANGELOG.md) - 版本变更历史

## 🧪 测试

```bash
cd build
ctest
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
- [ ] 添加健康检查端点
- [ ] 添加 Prometheus 指标导出
- [ ] 添加 Kubernetes ConfigMap 支持
- [ ] 添加服务发现集成
- [ ] 添加分布式追踪支持

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