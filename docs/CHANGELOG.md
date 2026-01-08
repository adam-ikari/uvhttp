# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2026-01-07

### Added
- **TLS安全功能**: 实现CRL检查、DH参数设置、会话票证管理、证书链验证
- **性能基准测试**: 新增性能基准测试程序和文档
- **安全策略文档**: 完整的安全策略和依赖管理文档
- **mimalloc支持**: 启用mimalloc作为默认内存分配器
- **原生WebSocket**: 使用原生WebSocket实现替代libwebsockets

### Changed
- **TLS实现**: 从OpenSSL迁移到mbedTLS，提升安全性和性能
- **性能优化**: Keep-alive连接管理优化，性能提升约1000倍（4-16 RPS → 14,000-16,000 RPS）
- **代码精简**: 移除15,192行冗余代码，提升代码可维护性
- **文档重组**: 移动文档到docs/目录，优化项目结构
- **依赖管理**: 清理.gitmodules，移除不再使用的依赖

### Fixed
- **空指针检查**: 修复request getter函数的空指针检查（11个函数）
- **TLS类型错误**: 修复mbedTLS API类型不兼容问题
- **编译警告**: 修复所有编译警告（未使用变量、strncpy截断等）
- **HTTP头验证**: 修复HTTP头验证逻辑错误
- **响应构建**: 修复响应数据构建时机问题

### Security
- **CRL检查**: 实现证书撤销列表检查功能
- **证书链验证**: 完整的证书链验证支持
- **依赖更新策略**: 明确的安全更新、功能更新、维护更新策略
- **安全审计**: 建立每周依赖扫描、每月代码审查、季度渗透测试计划
- **漏洞响应**: 定义完整的漏洞发现、评估、修复、通知流程

### Performance
- **Keep-alive连接**: 修复连接复用，性能提升约1000倍
- **TCP优化**: 启用TCP_NODELAY和TCP keepalive
- **响应缓冲区**: 优化响应缓冲区大小（512 → 1024字节）
- **内存分配器**: mimalloc提供更快的内存分配和释放

### Testing
- **NULL参数测试**: 完整的NULL参数覆盖测试（TLS 32个，Request 11个）
- **性能验证**: 1000倍性能提升已通过基准测试验证
- **代码质量**: 编译无错误无警告，启用安全编译选项

### Documentation
- **PERFORMANCE_BENCHMARK.md**: 详细的性能基准测试文档
- **SECURITY.md**: 完整的安全策略文档
- **依赖文档**: 更新DEPENDENCIES.md，包含所有依赖版本和更新策略

### Breaking Changes
- **TLS API变更**: 从OpenSSL迁移到mbedTLS，API签名有所变化
- **WebSocket实现**: 从libwebsockets迁移到原生实现
- **依赖变更**: 移除libwebsockets依赖，使用mbedTLS替代OpenSSL

## [1.1.0] - 2025-12-25

### Added
- **内存安全增强**: 使用C99灵活数组成员解决内存对齐问题
- **整数溢出保护**: 在所有关键内存分配点添加整数溢出检查
- **路径遍历防护**: 多层路径遍历保护机制，防止目录遍历攻击
- **性能优化**: 增加连接数限制（2048）和监听队列（1024）
- **新常量定义**: 添加HTTP状态码范围常量和响应头安全边距常量
- **文档**: 新增ROUTER_SEARCH_MODES.md文档
- **WebSocket全局清理**: 完善WebSocket全局资源清理函数

### Changed
- **API统一**: 统一所有验证函数返回值（1表示有效/成功，0表示无效/失败）
- **函数命名**: 移除所有兼容性包装函数，使用统一的API命名
- **代码清理**: 移除所有兼容性代码和注释
- **测试更新**: 更新所有测试以匹配新的API签名
- **错误处理**: 简化错误处理逻辑，提高代码可读性

### Fixed
- **内存对齐**: 修复uvhttp_write_data_t结构的内存对齐问题
- **内存泄漏**: 修复错误处理路径中的内存泄漏
- **危险字符检查**: 移除错误null字符检查逻辑
- **函数返回值**: 修正验证函数的返回值逻辑
- **编译错误**: 修复所有示例程序和测试程序的编译错误

### Security
- **缓冲区溢出保护**: 添加完整的边界检查和整数溢出检查
- **HTTP响应拆分防护**: 增强控制字符检测
- **DoS防护**: 合理的资源限制和超时配置
- **TLS支持**: 支持TLS 1.3加密协议
- **输入验证**: 28处输入验证调用，覆盖所有用户输入点

### Performance
- **哈希算法**: 使用xxHash高性能哈希算法
- **LRU缓存**: 优化路由缓存和静态文件缓存
- **零拷贝**: 减少内存拷贝操作
- **连接池**: 优化连接复用和管理

### Testing
- **单元测试**: 16/16测试通过（100%通过率）
- **测试覆盖**: 47%测试代码量
- **边界测试**: 完整的边界条件和极端条件测试
- **安全测试**: 路径遍历、DoS防护等安全测试

### Documentation
- **API文档**: 完整的API参考文档
- **开发指南**: 详细的开发指南和规范
- **架构文档**: 清晰的架构说明
- **示例代码**: 15个示例程序

## [1.0.0] - 2025-12-20

### Added
- **初始发布**: 基于libuv的高性能HTTP服务器
- **路由系统**: 支持动态路由和参数提取
- **静态文件服务**: 高效的静态文件服务
- **WebSocket支持**: 完整的WebSocket协议支持
- **TLS/SSL**: 支持HTTPS加密连接
- **LRU缓存**: 高性能缓存系统
- **连接池**: 优化连接复用
- **配置系统**: 灵活的配置管理
- **错误处理**: 完善的错误处理和日志系统

### Features
- 单线程事件驱动模型
- 高性能异步I/O
- 模块化设计
- 可扩展的插件系统
- 详细的文档和示例

[1.1.0]: https://github.com/adam-ikari/uvhttp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/adam-ikari/uvhttp/releases/tag/v1.0.0