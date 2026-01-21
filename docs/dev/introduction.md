# UVHTTP 开发者文档

欢迎使用 UVHTTP 开发者文档！本文档面向贡献者、维护者和希望深入了解 UVHTTP 内部实现的开发者。

## 📚 文档导航

### 开发指南
- [开发者指南](../DEVELOPER_GUIDE.md) - 开发者必读指南
- [开发环境搭建](./setup.md) - 配置开发环境
- [代码规范](./coding-standards.md) - 代码风格和规范
- [贡献指南](./contributing.md) - 如何贡献代码

### 架构设计
- [架构设计](../ARCHITECTURE.md) - 系统架构概览
- [模块设计](./modules.md) - 模块设计和职责
- [路由搜索模式](../ROUTER_SEARCH_MODES.md) - 路由匹配算法
- [统一响应指南](../UNIFIED_RESPONSE_GUIDE.md) - 响应处理设计

### 测试
- [测试指南](../TESTABILITY_GUIDE.md) - 测试指南
- [测试标准](../TESTING_STANDARDS.md) - 测试标准和覆盖率
- [性能测试标准](../PERFORMANCE_TESTING_STANDARD.md) - 性能测试规范

### 性能分析
- [性能分析](./performance-analysis.md) - 性能分析和优化
- [内存分析](./memory-analysis.md) - 内存管理和优化

### 开发工具
- [依赖管理](../DEPENDENCIES.md) - 第三方依赖
- [构建系统](./build-system.md) - CMake 构建系统
- [调试技巧](./debugging.md) - 调试方法和工具

### 开发计划
- [开发计划](../DEVELOPMENT_PLAN.md) - 当前开发计划
- [路线图](../ROADMAP.md) - 未来规划
- [变更日志](../CHANGELOG.md) - 版本变更历史

### 重构计划
- [全局变量重构计划](../GLOBAL_VARIABLE_REFACTOR_PLAN.md) - 全局变量重构
- [libuv 数据指针](../LIBUV_DATA_POINTER.md) - libuv 数据指针模式
- [xxhash 集成](../XXHASH_INTEGRATION.md) - xxhash 哈希算法集成

## 🎯 你是哪种开发者？

### 新贡献者
如果你是第一次贡献代码，建议按以下顺序阅读：
1. [开发者指南](../DEVELOPER_GUIDE.md)
2. [开发环境搭建](./setup.md)
3. [代码规范](./coding-standards.md)
4. [贡献指南](./contributing.md)
5. [测试指南](../TESTABILITY_GUIDE.md)

### 核心开发者
如果你是核心开发者，需要深入了解：
1. [架构设计](../ARCHITECTURE.md)
2. [模块设计](./modules.md)
3. [性能分析](./performance-analysis.md)
4. [内存分析](./memory-analysis.md)
5. [开发计划](../DEVELOPMENT_PLAN.md)

### 维护者
如果你是项目维护者，需要关注：
1. [路线图](../ROADMAP.md)
2. [变更日志](../CHANGELOG.md)
3. [性能测试标准](../PERFORMANCE_TESTING_STANDARD.md)
4. [重构计划](../GLOBAL_VARIABLE_REFACTOR_PLAN.md)

## 💡 推荐阅读路径

### 路径 1：贡献新功能
1. [开发者指南](../DEVELOPER_GUIDE.md)
2. [开发环境搭建](./setup.md)
3. [代码规范](./coding-standards.md)
4. [架构设计](../ARCHITECTURE.md)
5. [测试指南](../TESTABILITY_GUIDE.md)
6. [贡献指南](./contributing.md)

### 路径 2：性能优化
1. [性能基准](../PERFORMANCE_BENCHMARK.md)
2. [性能分析](./performance-analysis.md)
3. [内存分析](./memory-analysis.md)
4. [路由搜索模式](../ROUTER_SEARCH_MODES.md)
5. [性能测试标准](../PERFORMANCE_TESTING_STANDARD.md)

### 路径 3：重构和改进
1. [架构设计](../ARCHITECTURE.md)
2. [重构计划](../GLOBAL_VARIABLE_REFACTOR_PLAN.md)
3. [模块设计](./modules.md)
4. [测试标准](../TESTING_STANDARDS.md)
5. [开发计划](../DEVELOPMENT_PLAN.md)

## 🔧 开发工作流

### 1. 设置开发环境
```bash
# 克隆仓库
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 安装依赖
git submodule update --init --recursive

# 构建项目
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行测试
./run_tests.sh
```

### 2. 开发新功能
1. 创建功能分支：`git checkout -b feature/your-feature`
2. 编写代码（遵循 [代码规范](./coding-standards.md)）
3. 编写测试（遵循 [测试标准](../TESTING_STANDARDS.md)）
4. 运行测试：`./run_tests.sh`
5. 提交代码：`git commit -m "feat: add your feature"`
6. 推送到远程：`git push origin feature/your-feature`
7. 创建 Pull Request

### 3. 性能测试
```bash
# 运行性能测试
cd test/performance
./run_performance_tests.sh

# 使用 wrk 测试
wrk -t4 -c100 -d30s http://localhost:8080/
```

### 4. 代码审查
- 遵循 [贡献指南](./contributing.md)
- 确保所有测试通过
- 确保代码覆盖率不低于 80%
- 确保没有编译警告

## 📊 代码质量标准

- **测试覆盖率**: ≥ 80%
- **编译警告**: 0
- **代码规范**: 遵循项目规范
- **文档**: 所有公开 API 必须有文档
- **性能**: 不低于基线性能

## 🚀 开始贡献

准备开始贡献了吗？从 [开发者指南](../DEVELOPER_GUIDE.md) 开始吧！

## 📞 联系方式

- GitHub Issues: https://github.com/adam-ikari/uvhttp/issues
- GitHub Discussions: https://github.com/adam-ikari/uvhttp/discussions