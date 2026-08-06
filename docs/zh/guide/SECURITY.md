---
title: 安全
description: UVHTTP 安全特性 — 缓冲区溢出保护、HTTP 响应拆分防护、输入验证、通过 mbedtls 实现 TLS 1.3、安全默认值，以及 ASan/UBSan 内存安全验证工作流。
---

# 安全策略

## 概述

本文档涵盖 UVHTTP 安全策略：依赖管理、漏洞响应和安全实践。

## 依赖管理

### 当前依赖

| 依赖 | 版本（.gitmodules 中） | 用途 | 更新策略 |
|------------|--------------------------|---------|---------------|
| libuv | git submodule | 事件循环 | 定期更新 |
| mbedtls | git submodule | TLS/SSL | 安全更新优先 |
| mimalloc | git submodule | 内存分配器 | 定期更新 |
| cjson | git submodule | JSON 解析 | 定期更新 |
| llhttp | git submodule | HTTP 解析 | 定期更新 |
| uthash | git submodule | 哈希表 | 定期更新 |
| xxhash | git submodule | 快速哈希 | 定期更新 |
| googletest | git submodule | 测试框架 | 按需更新 |

### 依赖更新策略

#### 1. 安全更新（高优先级）
- **触发条件**: 发现 CVE 漏洞或严重安全问题
- **响应时间**: 7 天内评估，14 天内修复
- **流程**:
  1. 评估漏洞影响范围
  2. 检查上游修复版本
  3. 更新依赖版本
  4. 运行完整测试套件
  5. 发布安全补丁版本

#### 2. 功能更新（中优先级）
- **触发条件**: 新功能、性能改进、API 变更
- **响应时间**: 季度评估
- **流程**:
  1. 评估新功能价值
  2. 检查 API 兼容性
  3. 更新依赖版本
  4. 更新相关文档
  5. 发布次要版本

#### 3. 维护更新（低优先级）
- **触发条件**: 依赖版本过时（> 1 年）
- **响应时间**: 半年评估
- **流程**:
  1. 检查兼容性
  2. 更新依赖版本
  3. 运行测试
  4. 发布补丁版本

### 依赖版本固定

所有依赖版本都在 `.gitmodules` 中固定，以确保可重复构建。

**优势**:
- 可重复构建
- 避免意外的破坏性变更
- 更容易追踪问题

**劣势**:
- 需要手动更新依赖
- 可能错过安全更新

**缓解措施**:
- 定期安全扫描
- 订阅安全公告
- 建立自动化检查

## 安全审计

### 定期审计计划

| 审计类型 | 频率 | 负责人 |
|------------|-----------|-------|
| 依赖漏洞扫描 | 每周 | 自动化 |
| 代码安全审查 | 每月 | 安全团队 |
| 渗透测试 | 每季度 | 第三方 |
| 架构安全审查 | 每半年 | 安全团队 |

### 自动化安全扫描

使用以下工具进行自动化安全扫描：

1. **依赖扫描**
   ```bash
   # 使用 GitHub Dependabot
   # 配置文件: .github/dependabot.yml
   ```

2. **静态代码分析**
   ```bash
   # 使用 cppcheck
cppcheck --enable=all src/
   ```

3. **内存安全检查**
   ```bash
   # 使用 Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./uvhttp_server
   ```

4. **AddressSanitizer（泄漏、释放后使用、溢出）**
   ```bash
   cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
   cmake --build build_asan -j$(nproc)
   (cd build_asan && ctest --output-on-failure)   # 完整套件，泄漏检测开启
   ```

5. **UndefinedBehaviorSanitizer（有符号溢出、移位、对齐等）**
   ```bash
   cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON ..
   cmake --build build_ubsan -j$(nproc)
   (cd build_ubsan && ctest --output-on-failure)
   ```
   ASan 和 UBSan 不能在同一个构建中组合使用；需要分别运行。

## 安全最佳实践

### 输入验证

UVHTTP 验证输入：

1. **URL 验证**
   - 最大 URL 长度：2048 字节
   - 路径遍历保护
   - URL 编码验证

2. **头部验证**
   - 最大头部数量：64
   - 最大头部名称长度：256 字节
   - 最大头部值长度：4096 字节

3. **请求体大小限制**
   - 最大请求体大小：10MB（可配置）
   - 分块传输编码支持

### 缓冲区溢出保护

所有字符串操作使用安全函数：

```c
// 安全字符串复制
if (uvhttp_safe_strcpy(dest, sizeof(dest), src) != 0) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

// 安全字符串长度
size_t len = strlen(src);
if (len >= sizeof(dest)) {
    len = sizeof(dest) - 1;
}
strncpy(dest, src, len);
dest[len] = '\0';
```

### TLS 配置

推荐的 TLS 配置：

```c
// 仅启用 TLS 1.3
mbedtls_ssl_conf_min_tls_version(&conf, MBEDTLS_SSL_TLS_1_3);

// 启用证书验证
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);

// 设置安全密码套件
const int ciphers[] = {
    MBEDTLS_TLS_AES_256_GCM_SHA384,
    MBEDTLS_TLS_CHACHA20_POLY1305_SHA256,
    0
};
mbedtls_ssl_conf_ciphersuites(&conf, ciphers);
```

### DoS 防护

UVHTTP 应用多项 DoS 防护措施：

1. **限流**
   - 令牌桶算法
   - 每 IP 可配置限制
   - 白名单支持

2. **连接限制**
   - 最大连接数：2048（可配置）
   - 连接超时：60 秒
   - 请求超时：30 秒

3. **资源限制**
   - 最大请求体大小：10MB
   - 最大头部大小：8KB
   - 每连接最大并发请求数：100

## 漏洞报告

### 报告流程

如果发现安全漏洞，请负责任地报告：

1. **不要创建公开 issue**
2. **发送邮件至**: security@uvhttp.org
3. **包含**: 漏洞描述、复现步骤、受影响版本
4. **响应时间**: 我们将在 48 小时内响应

### 漏洞处理流程

1. **确认**（48 小时内）
   - 确认收到报告
   - 分配严重等级
   - 预估修复时间

2. **评估**（7 天内）
   - 复现漏洞
   - 评估影响
   - 开发修复

3. **修复开发**（14 天内）
   - 实现修复
   - 编写测试
   - 代码审查

4. **发布**（21 天内）
   - 准备安全公告
   - 发布补丁版本
   - 协调披露

### 严重等级

| 严重等级 | 说明 | 响应时间 |
|----------|-------------|---------------|
| 严重 | 远程代码执行 | 48 小时 |
| 高 | 数据泄露或 DoS | 7 天 |
| 中 | 信息泄露 | 14 天 |
| 低 | 轻微安全问题 | 30 天 |

## 安全特性

### 内存安全

- **Sanitizer 验证**: 完整的 91 项测试套件在 AddressSanitizer（泄漏检测开启 — 零泄漏、零释放后使用、零缓冲区溢出）和 UndefinedBehaviorSanitizer（零未定义行为）下通过。参见 `.github/workflows/ci-nightly.yml`（`test-memory` + `test-ubsan` 任务）。
- **零编译警告**: 所有代码使用 `-Werror` 编译
- **内存分配器**: mimalloc 提升内存安全（可选；也支持系统分配器）
- **缓冲区溢出保护**: 所有字符串操作均已验证
- **内存泄漏检测**: 定期的 Valgrind 和 AddressSanitizer 测试

### 输入验证

- **URL 验证**: 长度限制、路径遍历保护
- **头部验证**: 大小限制、格式验证
- **请求体验证**: 大小限制、编码验证
- **参数验证**: 类型检查、范围验证

### 安全默认值

- **TLS 1.3**: 使用 TLS 时默认启用
- **证书验证**: 默认要求
- **安全密码套件**: 预配置
- **限流**: 默认启用

## 安全检查清单

部署到生产环境前，确保：

- [ ] 所有依赖都是最新的
- [ ] 依赖中没有已知漏洞
- [ ] TLS 已正确配置
- [ ] 限流已启用
- [ ] 输入验证是全面的
- [ ] 错误消息不泄露敏感信息
- [ ] 日志不暴露敏感数据
- [ ] 文件权限正确
- [ ] 防火墙规则已配置
- [ ] 监控和告警已设置

## 安全资源

- **安全公告**: https://github.com/adam-ikari/uvhttp/security/advisories
- **CVE 数据库**: https://cve.mitre.org/
- **OWASP Top 10**: https://owasp.org/www-project-top-ten/
- **安全最佳实践**: https://wiki.sei.cmu.edu/confluence/display/seccode/Top+10+CERT+C+Coding+Rules（需要登录）
- **替代**: https://www.cert.org/confluence/display/seccode/Top+10+CERT+C+Coding+Rules

## 联系方式

安全相关问题或漏洞报告：
- **邮件**: security@uvhttp.org
- **GitHub 安全**: https://github.com/adam-ikari/uvhttp/security
- **PGP 密钥**: 可应要求提供

---

**最后更新**: 2026-02-02
**版本**: 1.0
**维护者**: UVHTTP 安全团队
