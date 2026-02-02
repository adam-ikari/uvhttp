# UVHTTP 文档结构

本文档说明 UVHTTP 项目的文档组织结构。

## 📚 文档分类

UVHTTP 文档分为两大类，分别面向不同的用户群体：

### 1. 使用者文档 (`/guide/`)

**目标用户**: 使用 UVHTTP 构建应用的开发者

**内容**:
- 快速开始指南
- 基础和高级教程
- API 参考文档
- 最佳实践
- 性能优化指南
- 常见问题解答

**导航**: [使用者文档](/guide/)

### 2. 开发者文档 (`/dev/`)

**目标用户**: 贡献者、维护者和希望深入了解内部实现的开发者

**内容**:
- 开发指南
- 架构设计
- 模块设计
- 测试标准
- 性能分析
- 开发工具
- 重构计划

**导航**: [开发者文档](/dev/)

## 📂 文档目录结构

```
docs/
├── guide/                    # 使用者文档
│   ├── introduction.md       # 使用者文档索引
│   ├── installation.md       # 安装指南
│   ├── getting-started.md    # 快速开始
│   ├── first-server.md       # 第一个服务器
│   ├── routing.md            # 路由系统
│   ├── requests.md           # 请求处理
│   ├── responses.md          # 响应处理
│   ├── websocket.md          # WebSocket
│   ├── unified-api.md        # 统一 API
│   ├── performance.md        # 性能优化
│   ├── faq.md                # 常见问题
│   └── best-practices.md     # 最佳实践
│
├── dev/                      # 开发者文档
│   ├── introduction.md       # 开发者文档索引
│   ├── setup.md              # 开发环境搭建
│   ├── coding-standards.md   # 代码规范
│   ├── contributing.md       # 贡献指南
│   ├── modules.md            # 模块设计
│   ├── performance-analysis.md # 性能分析
│   ├── memory-analysis.md    # 内存分析
│   ├── build-system.md       # 构建系统
│   └── debugging.md          # 调试技巧
│
├── api/                      # API 文档
│   └── introduction.md       # API 介绍
│
├── .vitepress/               # VitePress 配置
│   ├── config.ts             # 网站配置
│   └── sidebar.js            # 侧边栏配置
│
├── API_REFERENCE.md          # 完整 API 参考
├── ARCHITECTURE.md           # 架构设计
├── CHANGELOG.md              # 变更日志
├── CONTRIBUTING.md           # 贡献指南
├── DEPENDENCIES.md           # 依赖说明
├── DEVELOPER_GUIDE.md        # 开发者指南
├── DEVELOPMENT_PLAN.md       # 开发计划
├── PERFORMANCE_BENCHMARK.md  # 性能基准
├── PERFORMANCE_TESTING_STANDARD.md # 性能测试标准
├── RATE_LIMIT_API.md         # 限流 API
├── README.md                 # 项目 README
├── ROADMAP.md                # 路线图
├── ROUTER_SEARCH_MODES.md    # 路由搜索模式
├── SECURITY.md               # 安全指南
├── STATIC_FILE_SERVER.md     # 静态文件服务
├── TUTORIAL.md               # 教程
├── XXHASH_INTEGRATION.md     # xxhash 集成
└── LIBUV_DATA_POINTER.md     # libuv 数据指针模式
```

## 🎯 如何找到你需要的文档

### 如果你是应用开发者

1. **首次使用**: 从 [快速开始](/guide/getting-started) 开始
2. **学习 API**: 查看 [API 参考](/api/introduction)
3. **遇到问题**: 查看 [常见问题](/guide/faq)
4. **性能优化**: 阅读 [性能优化指南](/guide/performance)
5. **完整教程**: 学习 [完整教程](/TUTORIAL.md)

### 如果你是贡献者

1. **首次贡献**: 阅读 [开发者指南](/DEVELOPER_GUIDE.md)
2. **环境搭建**: 查看 [开发环境搭建](/dev/setup)
3. **代码规范**: 遵循 [代码规范](/dev/coding-standards)
4. **提交代码**: 参考 [贡献指南](/dev/contributing)
5. **测试要求**: 了解 [测试标准](/TESTABILITY_STANDARDS.md)

### 如果你是维护者

1. **架构设计**: 深入理解 [架构设计](/ARCHITECTURE.md)
2. **性能分析**: 查看 [性能分析](/dev/performance-analysis)
3. **开发计划**: 了解 [开发计划](/DEVELOPMENT_PLAN.md)
4. **重构计划**: 查看 [重构计划](/dev/)
5. **路线图**: 了解 [未来规划](/ROADMAP.md)

## 📝 文档编写指南

### 使用者文档编写要点

- **面向用户**: 从用户角度出发，解决实际问题
- **示例丰富**: 提供完整可运行的代码示例
- **循序渐进**: 从简单到复杂，逐步深入
- **实用性强**: 提供最佳实践和常见解决方案
- **清晰易懂**: 避免过于技术化的描述

### 开发者文档编写要点

- **技术深入**: 详细解释技术细节和设计决策
- **架构清晰**: 说明模块之间的关系和职责
- **标准明确**: 明确测试、代码质量等标准
- **可维护性**: 帮助新贡献者快速上手
- **性能导向**: 关注性能分析和优化方法

## 🔗 相关链接

- [主项目](https://github.com/adam-ikari/uvhttp)
- [使用指南](/guide/)
- [开发指南](/dev/)
- [API 参考](/API_REFERENCE.md)
- [贡献指南](/dev/contributing.md)

## 💬 反馈

如果你对文档有任何建议或发现问题，欢迎：
- 提交 [Issue](https://github.com/adam-ikari/uvhttp/issues)
- 参与 [Discussions](https://github.com/adam-ikari/uvhttp/discussions)
- 提交 Pull Request 改进文档