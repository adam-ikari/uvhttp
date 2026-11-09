# uvhttp 依赖管理文档

这里记录 uvhttp 用到的第三方库、锁定的版本，以及各版本之间的兼容范围。升级依赖前先看这份清单。

## 核心依赖

### 1. libuv

libuv 是事件循环的核心，负责异步 I/O。版本锁定在 v1.51.0，许可证为 MIT，属于必需依赖。

### 2. mbedtls

TLS/SSL 只由 mbedtls 提供，版本锁定在 v3.6.0，许可证为 Apache 2.0。HTTPS 和 mTLS 都走这一套实现，项目不引入第二个 TLS 库。

### 3. llhttp

HTTP 报文解析交给 llhttp，以 Git 子模块的形式引入，许可证为 MIT。当前跟随最新稳定版 (9.3.1)。它是生成式解析器，构建前要先运行 `npm install` 和 `make build/libllhttp.a`，把语法文件生成 C 源文件。

### 4. xxhash

xxhash 用于哈希计算，版本锁定在 v0.7.4，许可证为 BSD 2-Clause，属于必需依赖。

### 5. uthash

uthash 提供哈希表数据结构，是纯头文件库，版本锁定在 v1.9.8，许可证为 BSD Revised。

### WebSocket 实现

WebSocket 由 `src/uvhttp_websocket.c` 原生实现，不依赖第三方 WebSocket 库。代码量小，行为可控，构建时也不用多带一个依赖。

## 可选依赖

### 7. cjson

cjson 处理 JSON 数据，版本锁定在 v1.7.15，许可证为 MIT。它属于可选依赖，只有用到 JSON 接口时才需要。

### 8. mimalloc

mimalloc 是一个可选的内存分配器，版本锁定在 v3.1.5，许可证为 MIT。代码已经支持，但默认不启用，构建时通过 BUILD_WITH_MIMALLOC 选项打开。

## 测试依赖

### 9. googletest

单元测试使用 googletest，版本锁定在 release-1.12.1，许可证为 BSD 3-Clause。这个版本从 1.8.0 升级而来。

## 已移除的依赖

### cmocka

cmocka 已经移除。项目实际用的是 googletest，cmocka 从未派上用场，移除时间是 2026-01-05。

## 依赖升级策略

### 升级原则

1. 安全漏洞立即升级，不等例行维护窗口。
2. 升级前做完整的兼容性测试。
3. 所有依赖使用固定版本，避免构建时被动更新。
4. 先升级测试依赖，确认框架没问题，再动核心依赖。

### 升级流程

1. 先看新版本的变更日志和安全公告，确认有没有影响当前用法。
2. 在开发分支上做升级。
3. 跑完整的测试套件。
4. 验证性能指标没有回退。
5. 更新本文档里的版本号和兼容范围。
6. 提交 PR，走代码审查。

### 版本兼容性矩阵

| 依赖 | 当前版本 | 最低兼容版本 | 推荐版本 |
|------|---------|-------------|---------|
| libuv | v1.51.0 | v1.44.0 | v1.51.0 |
| mbedtls | v3.6.0 | v3.0.0 | v3.6.0 |
| llhttp | latest | v6.0.0 | latest |
| xxhash | v0.7.4 | v0.7.0 | v0.7.4 |
| uthash | v1.9.8 | v1.9.0 | v1.9.8 |
| cjson | v1.7.15 | v1.7.0 | v1.7.15 |
| mimalloc | v3.1.5 | v2.0.0 | v3.1.5 |
| googletest | release-1.12.1 | release-1.10.0 | release-1.12.1 |

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
make build
```

### 调试构建
```bash
make build
```

### 覆盖率构建
```bash
make build
```

### 使用 mimalloc
```bash
make build
```

### 组合选项
```bash
make build
```

## 安全考虑

### 当前安全措施

TLS 用 mbedtls，体积小，维护也活跃。编译时打开了一组加固选项：

- `-D_FORTIFY_SOURCE=2`
- `-fstack-protector-strong`
- `-Wformat-security`
- `-fno-omit-frame-pointer`
- `-Werror=implicit-function-declaration`
- `-Werror=format-security`
- `-Werror=return-type`

链接阶段启用只读重定位和立即绑定（`-Wl,-z,relro`、`-Wl,-z,now`），减少 GOT 被改写的窗口。googletest 已升级到修复过已知漏洞的稳定版本，所有依赖都锁了版本。

### 安全审计建议

1. 定期查看依赖库的安全公告。
2. 用依赖扫描工具（如 `npm audit`、`snyk` 等）检查已知问题。
3. 每季度做一次依赖安全审计。
4. 关注 CVE 数据库里和这些库相关的条目。
5. 定期更新 mbedtls 到最新稳定版本。

## 依赖更新日志

### 2026-01-05

googletest 从 release-1.8.0 升到 release-1.12.1；移除了一直没用上的 cmocka；TLS 统一到 mbedtls，去掉 OpenSSL；所有依赖加上版本锁定；同时补了安全编译选项。

## 联系方式

依赖相关的问题，联系项目维护者或提交 Issue。
