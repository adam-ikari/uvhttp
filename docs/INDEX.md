# UVHTTP 文档索引

本文档提供了 UVHTTP 项目的完整文档索引，帮助您快速找到所需的信息。

## 📚 核心文档

### 入门指南
- [快速开始](guide/getting-started.md) - 快速安装和运行 UVHTTP
- [安装指南](guide/installation.md) - 详细的安装说明
- [第一个服务器](guide/first-server.md) - 创建第一个 HTTP 服务器
- [教程](guide/TUTORIAL.md) - 从基础到高级的渐进式教程
- [开发者指南](guide/DEVELOPER_GUIDE.md) - 开发指南和最佳实践

### API 文档
- [API 参考](../docs/API_REFERENCE.md) - 完整的 API 文档
- [统一响应处理](guide/UNIFIED_RESPONSE_GUIDE.md) - 响应处理标准模式

### 架构和设计
- [架构设计](ARCHITECTURE.md) - 系统架构说明
- [设计原则](IFLOW.md) - 项目设计原则和约定

## 🔧 功能模块

### WebSocket
- [WebSocket 指南](guide/websocket.md) - WebSocket 使用指南
- [连接管理](../examples/05_websocket/test_ws_connection_management.c) - WebSocket 连接管理示例

### 静态文件
- [静态文件服务](guide/STATIC_FILE_SERVER.md) - 静态文件服务指南
- [缓存优化](../examples/04_static_files/cache_test_server.c) - 缓存优化示例

### 限流
- [限流 API](guide/RATE_LIMIT_API.md) - 限流功能 API 文档
- [限流示例](../examples/03_middleware/rate_limit_demo.c) - 限流中间件示例

### 中间件
- [中间件系统](MIDDLEWARE_SYSTEM.md) - 中间件系统使用指南
- [中间件链示例](../examples/03_middleware/middleware_chain_demo.c) - 中间件链示例

## 🎯 示例程序

完整的示例程序列表和说明请查看 [示例程序总览](../examples/README.md)。

### 基础示例 (01_basics)
- [Hello World](../examples/01_basics/01_hello_world.c) - 最简单的 HTTP 服务器
- [完整示例](../examples/01_basics/helloworld_complete.c) - 带完整错误处理的示例
- [快速 API 演示](../examples/01_basics/quick_api_demo.c) - 快速 API 演示

### 路由示例 (02_routing)
- [简单路由](../examples/02_routing/01_simple_routing.c) - URL 路由示例
- [方法路由](../examples/02_routing/02_method_routing.c) - HTTP 方法路由

### 中间件示例 (03_middleware)
- [编译时中间件](../examples/03_middleware/middleware_compile_time_demo.c) - 编译时中间件示例
- [中间件链](../examples/03_middleware/middleware_chain_demo.c) - 中间件链示例
- [限流中间件](../examples/03_middleware/rate_limit_demo.c) - 限流中间件示例

### 静态文件示例 (04_static_files)
- [静态文件服务器](../examples/04_static_files/static_file_server.c) - 静态文件服务器
- [缓存测试](../examples/04_static_files/cache_test_server.c) - 缓存测试服务器
- [高级服务器](../examples/04_static_files/advanced_static_server.c) - 高级静态文件服务器

### WebSocket 示例 (05_websocket)
- [回显服务器](../examples/05_websocket/websocket_echo_server.c) - WebSocket 回显服务器
- [测试服务器](../examples/05_websocket/websocket_test_server.c) - WebSocket 测试服务器
- [连接管理](../examples/05_websocket/test_ws_connection_management.c) - 连接管理示例

### 高级示例 (06_advanced)
- [API 演示](../examples/06_advanced/api_demo.c) - REST API 演示
- [JSON API](../examples/06_advanced/json_api_demo.c) - JSON API 演示
- [配置管理](../examples/06_advanced/config_demo.c) - 配置管理演示
- [上下文注入](../examples/06_advanced/context_injection.c) - 上下文注入示例

## 🏗️ 构建和部署

### 构建
- [构建指南](BUILD_GUIDE.md) - 详细的构建说明
- [CMake 导入目标](CMAKE_IMPORTED_TARGETS_GUIDE.md) - CMake 导入目标指南
- [CMake 目标链接](CMAKE_TARGET_LINKING_GUIDE.md) - CMake 目标链接指南

### CI/CD
- [CI/CD 实现](CI_CD_IMPLEMENTATION_SUMMARY.md) - CI/CD 实现总结
- [分支策略](BRANCH_STRATEGY.md) - Git 分支策略
- [发布检查清单](RELEASE_CHECKLIST.md) - 发布前的检查清单
- [生命周期设计](LIFECYCLE_DESIGN.md) - 项目生命周期设计

## 📊 性能

### 性能测试
- [性能基准](PERFORMANCE_BENCHMARK.md) - 性能测试结果
- [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) - 性能测试规范
- [性能报告](performance.md) - 性能报告

### 性能优化
- [服务器配置性能指南](SERVER_CONFIG_PERFORMANCE_GUIDE.md) - 服务器性能配置

## 🔐 安全

- [安全指南](SECURITY.md) - 安全相关说明

## 📋 项目管理

### 贡献
- [贡献指南](CONTRIBUTING.md) - 如何贡献代码

### 变更历史
- [变更日志](CHANGELOG.md) - 版本变更历史

### 迁移
- [迁移指南](MIGRATION_GUIDE.md) - 从旧版本迁移的指南

## 🔍 故障排查

### 常见问题
- 查看 [GitHub Issues](https://github.com/adam-ikari/uvhttp/issues) - 已知问题和解决方案

## 📞 联系方式

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues
- 文档站点: https://adam-ikari.github.io/uvhttp/

## 📝 文档维护

本文档索引会随着项目的发展持续更新。如果您发现文档缺失或有改进建议，欢迎提交 Issue 或 Pull Request。