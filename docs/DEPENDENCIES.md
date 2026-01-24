# uvhttp 依赖管理文档

本文档记录了 uvhttp 项目使用的所有第三方依赖库的版本信息和兼容性说明。

## 核心依赖

### 1. libuv
- **版本**: v1.51.0
- **用途**: 异步 I/O 事件循环核心库
- **类型**: 必需依赖
- **许可证**: MIT
- **状态**: ✅ 已锁定

### 2. mbedtls
- **版本**: v3.6.0
- **用途**: TLS/SSL 加密支持
- **类型**: 必需依赖
- **许可证**: Apache 2.0
- **状态**: ✅ 已锁定
- **说明**: 项目使用 mbedtls 作为唯一的 TLS 库实现。所有 TLS 相关功能（包括 HTTPS、mTLS）都通过 mbedtls 实现。

### 3. llhttp (cllhttp)
- **版本**: 最新稳定版
- **用途**: HTTP 协议解析
- **类型**: 必需依赖
- **许可证**: MIT
- **状态**: ✅ 已使用

### 4. xxhash
- **版本**: v0.7.4
- **用途**: 高性能哈希计算
- **类型**: 必需依赖
- **许可证**: BSD 2-Clause
- **状态**: ✅ 已锁定

### 5. uthash
- **版本**: v1.9.8
- **用途**: 哈希表数据结构
- **类型**: 必需依赖（头文件库）
- **许可证**: BSD Revised
- **状态**: ✅ 已锁定

### WebSocket 实现
- **实现方式**: 原生实现（uvhttp_websocket.c）
- **说明**: 不依赖第三方 WebSocket 库，完全自主实现
- **优势**: 更轻量、更可控、无额外依赖

## 可选依赖

### 7. cjson
- **版本**: v1.7.15
- **用途**: JSON 数据处理
- **类型**: 可选依赖
- **许可证**: MIT
- **状态**: ✅ 已锁定

### 8. mimalloc
- **版本**: v3.1.5
- **用途**: 高性能内存分配器
- **类型**: 可选依赖（通过 BUILD_WITH_MIMALLOC 选项启用）
- **许可证**: MIT
- **状态**: ✅ 已锁定，代码中已支持但未默认启用

## 测试依赖

### 9. googletest
- **版本**: release-1.12.1
- **用途**: 单元测试框架
- **类型**: 测试依赖
- **许可证**: BSD 3-Clause
- **状态**: ✅ 已锁定（从 1.8.0 升级）

## 已移除的依赖

### cmocka
- **状态**: ❌ 已移除
- **原因**: 项目实际使用 googletest 作为测试框架，cmocka 未被使用
- **移除时间**: 2026-01-05

## 依赖升级策略

### 升级原则
1. **安全优先**: 发现安全漏洞时立即升级
2. **兼容性测试**: 升级前必须进行完整的兼容性测试
3. **版本锁定**: 所有依赖使用固定版本，避免意外更新
4. **渐进式升级**: 优先升级测试依赖，再升级核心依赖

### 升级流程
1. 检查新版本的变更日志和安全公告
2. 在开发分支进行升级
3. 运行完整的测试套件
4. 验证性能指标
5. 更新本文档
6. 提交 PR 并进行代码审查

### 版本兼容性矩阵

| 依赖 | 当前版本 | 最低兼容版本 | 推荐版本 |
|------|---------|-------------|---------|
| libuv | v1.51.0 | v1.44.0 | v1.51.0 |
| mbedtls | v3.6.0 | v3.0.0 | v3.6.0 |
| llhttp | latest | v6.0.0 | latest |
| xxhash | v0.7.4 | v0.7.0 | v0.7.4 |
| uthash | v1.9.8 | v1.9.0 | v1.9.8 |
| libwebsockets | v4.5.0 | v4.3.0 | v4.5.0 |
| cjson | v1.7.15 | v1.7.0 | v1.7.15 |
| mimalloc | v3.1.5 | v2.0.0 | v3.1.5 |
| googletest | release-1.12.1 | release-1.10.0 | release-1.12.1 |

**注意**: 项目使用 mbedtls 作为 TLS 库实现。libwebsockets 预编译库使用 OpenSSL，但项目自身的 TLS 功能完全基于 mbedtls。

## 编译选项

### 内存分配器选择

```bash
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### 日志系统

```bash
# 启用日志系统（默认）
cmake -DUVHTTP_FEATURE_LOGGING=ON ..

# 禁用日志系统（零开销）
cmake -DUVHTTP_FEATURE_LOGGING=OFF ..
```

## 构建选项

### 标准构建
```bash
mkdir build && cd build
cmake ..
make
```

### 调试构建
```bash
cmake -DENABLE_DEBUG=ON ..
make
```

### 覆盖率构建
```bash
cmake -DENABLE_COVERAGE=ON ..
make
```

### 使用 mimalloc
```bash
cmake -DBUILD_WITH_MIMALLOC=ON ..
make
```

### 组合选项
```bash
cmake -DENABLE_DEBUG=ON -DENABLE_COVERAGE=ON -DBUILD_WITH_MIMALLOC=ON ..
make
```

## 安全考虑

### 当前安全措施
- ✅ 使用 mbedtls 作为 TLS 库（轻量级、安全）
- ✅ 启用编译时安全标志：
  - `-D_FORTIFY_SOURCE=2`
  - `-fstack-protector-strong`
  - `-Wformat-security`
  - `-fno-omit-frame-pointer`
  - `-Werror=implicit-function-declaration`
  - `-Werror=format-security`
  - `-Werror=return-type`
- ✅ 启用链接器安全标志：
  - `-Wl,-z,relro` (只读重定位)
  - `-Wl,-z,now` (立即绑定)
- ✅ 升级 googletest 到最新稳定版本（修复已知漏洞）
- ✅ 所有依赖使用固定版本

### 安全审计建议
1. 定期检查依赖库的安全公告
2. 使用依赖扫描工具（如 `npm audit`、`snyk` 等）
3. 每季度进行一次依赖安全审计
4. 关注 CVE 数据库中的相关漏洞
5. 定期更新 mbedtls 到最新稳定版本

## 依赖更新日志

### 2026-01-05
- ✅ 升级 googletest: release-1.8.0 → release-1.12.1
- ✅ 移除 cmocka（未使用的测试框架）
- ✅ 统一 TLS 库为 mbedtls（移除 OpenSSL）
- ✅ 添加所有依赖的版本锁定
- ✅ 添加安全编译选项

## 联系方式

如有依赖相关问题，请联系项目维护者或提交 Issue。