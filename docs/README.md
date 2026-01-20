# UVHTTP 文档中心

欢迎使用 UVHTTP 文档中心！UVHTTP 是一个基于 libuv 的高性能、轻量级 HTTP/1.1 和 WebSocket 服务器库，采用 C11 标准编写。

## 📚 文档导航

### 🚀 快速开始

- **[教程](TUTORIAL.md)** - 从基础到高级的渐进式教程
  - 快速入门
  - 基础概念
  - 常用功能
  - 高级特性

- **[API 参考](API_REFERENCE.md)** - 完整的 API 文档
  - 核心函数
  - 数据结构
  - 错误码
  - 使用示例

### 🏗️ 架构与设计

- **[架构设计](ARCHITECTURE.md)** - 系统架构说明
  - 整体架构
  - 模块设计
  - 数据流
  - 扩展机制

- **[开发者指南](DEVELOPER_GUIDE.md)** - 开发指南和最佳实践
  - 开发环境
  - 代码规范
  - 调试技巧
  - 贡献指南

- **[依赖说明](DEPENDENCIES.md)** - 第三方依赖说明
  - libuv
  - llhttp
  - mbedtls
  - mimalloc

### ⚙️ 功能模块

- **[中间件系统](MIDDLEWARE_SYSTEM.md)** - 功能模块使用指南
  - 中间件概念
  - 内置中间件
  - 自定义中间件
  - 中间件链

- **[静态文件服务](STATIC_FILE_SERVER.md)** - 静态文件服务指南
  - 基本配置
  - 缓存策略
  - MIME 类型
  - 目录列表

- **[路由系统](ROUTER_SEARCH_MODES.md)** - 路由搜索模式
  - 路由匹配
  - 参数提取
  - 搜索优化
  - 路由缓存

- **[统一响应指南](UNIFIED_RESPONSE_GUIDE.md)** - 响应处理标准
  - 响应构建
  - 状态码
  - 响应头
  - 响应体

- **[限流 API](RATE_LIMIT_API.md)** - 速率限制功能
  - 限流配置
  - 限流策略
  - 限流中间件
  - 自定义限流

### 🔒 安全与错误处理

- **[安全指南](SECURITY.md)** - 安全相关说明
  - 安全特性
  - 最佳实践
  - 漏洞防护
  - 安全审计

- **[错误码参考](ERROR_CODES.md)** - 完整的错误码列表
  - 错误码分类
  - 错误描述
  - 错误处理
  - 错误恢复

- **[错误处理指南](ERROR_CODES.md#错误处理)** - 错误处理最佳实践
  - 错误检测
  - 错误恢复
  - 错误日志
  - 错误报告

### 🚀 性能优化

- **[性能测试标准](PERFORMANCE_TESTING_STANDARD.md)** - 性能测试标准
  - 测试环境标准
  - 测试工具标准
  - 测试方法标准
  - 性能指标标准
  - 报告格式标准

> **注意**: 性能基准测试请参考 `PERFORMANCE_TESTING_STANDARD.md` 中的测试标准和方法。

### 🧪 测试与质量

- **[可测试性指南](TESTABILITY_GUIDE.md)** - 测试指南
  - 单元测试
  - 集成测试
  - 性能测试
  - 测试覆盖率

### 🔧 高级特性

- **[libuv 数据指针](LIBUV_DATA_POINTER.md)** - 避免全局变量的最佳实践
  - 数据指针模式
  - 上下文管理
  - 使用示例
  - 最佳实践

- **[xxHash 集成](XXHASH_INTEGRATION.md)** - 哈希算法集成
  - xxHash 简介
  - 集成方法
  - 性能优势
  - 使用示例

### 📝 变更历史

- **[变更日志](CHANGELOG.md)** - 版本变更历史
  - 版本历史
  - 新增功能
  - 修复问题
  - 已知问题

## 📖 文档分类

### 按用途分类

#### 入门文档
- [教程](TUTORIAL.md) - 新手入门
- [API 参考](API_REFERENCE.md) - API 查阅
- [快速开始](TUTORIAL.md#快速入门) - 快速上手

#### 开发文档
- [架构设计](ARCHITECTURE.md) - 系统架构
- [开发者指南](DEVELOPER_GUIDE.md) - 开发规范
- [依赖说明](DEPENDENCIES.md) - 依赖管理

#### 功能文档
- [中间件系统](MIDDLEWARE_SYSTEM.md) - 中间件
- [静态文件服务](STATIC_FILE_SERVER.md) - 静态文件
- [路由系统](ROUTER_SEARCH_MODES.md) - 路由
- [限流 API](RATE_LIMIT_API.md) - 限流

#### 性能文档
- [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) - 测试标准

#### 安全文档
- [安全指南](SECURITY.md) - 安全特性
- [错误码参考](ERROR_CODES.md) - 错误处理

#### 测试文档
- [可测试性指南](TESTABILITY_GUIDE.md) - 测试指南

#### 高级文档
- [libuv 数据指针](LIBUV_DATA_POINTER.md) - 数据指针
- [xxHash 集成](XXHASH_INTEGRATION.md) - 哈希算法

#### 参考文档
- [变更日志](CHANGELOG.md) - 版本历史
- [统一响应指南](UNIFIED_RESPONSE_GUIDE.md) - 响应处理

### 按角色分类

#### 新手用户
1. [教程](TUTORIAL.md) - 学习基础概念
2. [API 参考](API_REFERENCE.md) - 查阅 API
3. [快速开始](TUTORIAL.md#快速入门) - 快速上手

#### 应用开发者
1. [API 参考](API_REFERENCE.md) - API 使用
2. [中间件系统](MIDDLEWARE_SYSTEM.md) - 功能集成
3. [静态文件服务](STATIC_FILE_SERVER.md) - 静态文件
4. [路由系统](ROUTER_SEARCH_MODES.md) - 路由配置
5. [限流 API](RATE_LIMIT_API.md) - 限流功能

#### 性能优化者
1. [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) - 了解性能测试方法

#### 核心开发者
1. [架构设计](ARCHITECTURE.md) - 理解架构
2. [开发者指南](DEVELOPER_GUIDE.md) - 开发规范
3. [依赖说明](DEPENDENCIES.md) - 依赖管理
4. [可测试性指南](TESTABILITY_GUIDE.md) - 测试方法
5. [libuv 数据指针](LIBUV_DATA_POINTER.md) - 最佳实践

#### 安全审计者
1. [安全指南](SECURITY.md) - 安全特性
2. [错误码参考](ERROR_CODES.md) - 错误处理
3. [架构设计](ARCHITECTURE.md) - 安全架构

## 🔍 快速查找

### 常见问题

**如何快速开始？**
→ 查看 [教程](TUTORIAL.md)

**如何查找 API？**
→ 查看 [API 参考](API_REFERENCE.md)

**如何优化性能？**
→ 查看 [服务器配置与性能优化指南](SERVER_CONFIG_PERFORMANCE_GUIDE.md)

**如何处理错误？**
→ 查看 [错误码参考](ERROR_CODES.md)

**如何使用中间件？**
→ 查看 [中间件系统](MIDDLEWARE_SYSTEM.md)

**如何服务静态文件？**
→ 查看 [静态文件服务](STATIC_FILE_SERVER.md)

**如何实现限流？**
→ 查看 [限流 API](RATE_LIMIT_API.md)

**如何进行性能测试？**
→ 查看 [性能测试标准](PERFORMANCE_TESTING_STANDARD.md)

**如何贡献代码？**
→ 查看 [开发者指南](DEVELOPER_GUIDE.md)

**如何确保安全？**
→ 查看 [安全指南](SECURITY.md)

### 按功能查找

| 功能 | 文档 |
|------|------|
| HTTP 服务器 | [API 参考](API_REFERENCE.md), [教程](TUTORIAL.md) |
| WebSocket | [API 参考](API_REFERENCE.md), [教程](TUTORIAL.md) |
| 静态文件 | [静态文件服务](STATIC_FILE_SERVER.md) |
| 路由 | [路由系统](ROUTER_SEARCH_MODES.md) |
| 中间件 | [中间件系统](MIDDLEWARE_SYSTEM.md) |
| 限流 | [限流 API](RATE_LIMIT_API.md) |
| TLS/HTTPS | [API 参考](API_REFERENCE.md), [依赖说明](DEPENDENCIES.md) |
| 缓存 | [静态文件服务](STATIC_FILE_SERVER.md) |
| 错误处理 | [错误码参考](ERROR_CODES.md) |
| 性能优化 | [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) |

### 按场景查找

| 场景 | 文档 |
|------|------|
| 快速入门 | [教程](TUTORIAL.md) |
| API 开发 | [API 参考](API_REFERENCE.md), [教程](TUTORIAL.md) |
| Web 应用 | [中间件系统](MIDDLEWARE_SYSTEM.md), [路由系统](ROUTER_SEARCH_MODES.md) |
| 静态网站 | [静态文件服务](STATIC_FILE_SERVER.md) |
| 高并发服务 | [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) |
| 安全应用 | [安全指南](SECURITY.md), [错误码参考](ERROR_CODES.md) |
| 性能测试 | [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) |
| 核心开发 | [架构设计](ARCHITECTURE.md), [开发者指南](DEVELOPER_GUIDE.md) |

### 📊 文档统计

- **总文档数**: 18
- **入门文档**: 3
- **开发文档**: 3
- **功能文档**: 5
- **性能文档**: 3
- **安全文档**: 2
- **测试文档**: 1
- **高级文档**: 2
- **参考文档**: 2

**性能文档说明**：
- `PERFORMANCE_TESTING_STANDARD.md` - 性能测试标准（包含测试计划）

**重要**：所有性能基准值都基于实际测试结果，不使用虚假的预期性能。

## 🔄 文档更新

### 最新更新

- **2026-01-10**
  - 新增 [服务器配置与性能优化指南](SERVER_CONFIG_PERFORMANCE_GUIDE.md)
  - 修正所有性能基准值，使用实际测试数据
  - 整合性能测试相关文档，明确文档定位和关系
  - 合并 [性能测试计划](PERFORMANCE_TESTING_PLAN.md) 到 [性能测试标准](PERFORMANCE_TESTING_STANDARD.md)
  - 删除冗余文档，减少文档数量（21 → 18）

### 更新日志

详细更新历史请查看 [变更日志](CHANGELOG.md)。

## 🤝 文档贡献

如果您发现文档有错误或需要改进，欢迎贡献！

### 贡献方式

1. Fork 项目
2. 创建分支 (`git checkout -b feature/doc-improvement`)
3. 提交更改 (`git commit -am 'Add new documentation'`)
4. 推送到分支 (`git push origin feature/doc-improvement`)
5. 创建 Pull Request

### 文档规范

- 使用 Markdown 格式
- 保持简洁清晰
- 提供代码示例
- 更新相关文档
- 遵循现有风格

## 📞 获取帮助

如果您在阅读文档或使用 UVHTTP 时遇到问题：

1. 查看 [教程](TUTORIAL.md) 和 [API 参考](API_REFERENCE.md)
2. 搜索 [已知问题](CHANGELOG.md)
3. 在 GitHub 提交 Issue
4. 查看示例代码 (`examples/` 目录)

## 🔗 相关链接

- **项目主页**: https://github.com/adam-ikari/uvhttp
- **问题反馈**: https://github.com/adam-ikari/uvhttp/issues
- **许可证**: MIT License

---

**UVHTTP 版本**: 1.5.0
**文档更新**: 2026-01-20
**维护者**: UVHTTP 团队