# UVHTTP 工程开发规范

## 1. 代码风格

### 1.1 C 语言规范

- **标准**：uvhttp 库源代码使用 C11 标准, 测试代码使用 C++17，实例代码包含 C 和 CPP 两个版本
- **缩进**：使用 4 个空格，不使用 Tab
- **大括号**：K&R 风格
- **行长度**：最大 120 字符
- **命名约定**：
  - 函数：`snake_case`（如 `uvhttp_request_init`）
  - 变量：`snake_case`（如 `request_count`）
  - 常量：`UPPER_SNAKE_CASE`（如 `UVHTTP_MAX_URL_SIZE`）
  - 类型定义：`snake_case_t`（如 `uvhttp_request_t`）
- **代码风格**：uvhttp 库源代码使用 Linux 内核代码风格，测试代码和实例代码使用 google cpp 风格

### 1.2 头文件组织

```c
/* 版权信息 */
/* 系统头文件 */
#include <stdio.h>
#include <stdlib.h>

/* 第三方库头文件 */
#include <uv.h>
#include <libwebsockets.h>

/* 项目内部头文件 */
#include "uvhttp_common.h"
#include "uvhttp_request.h"
```

### 1.3 函数文档

每个公共函数必须有文档注释：

```c
/**
 * @brief 初始化 HTTP 请求
 * @param request 请求结构体指针
 * @param client TCP 连接指针
 * @return 0 成功，-1 失败
 */
int uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
```

## 2. 错误处理

### 2.1 错误码定义

- 使用统一的错误码系统
- 错误码为负值，0 表示成功
- 错误码定义在 `uvhttp_error.h` 中

### 2.2 错误处理原则

uvhttp 的 release 版本中不输出任何 log，只有 debug 版本的时候会使用 LOG 宏开启 log 输出

```c
int result = some_function();
if (result != UVHTTP_ERROR_NONE) {
    UVHTTP_ERROR("Function failed: %s\n", uvhttp_error_string(result));
    return result;
}
```

## 3. 内存管理

### 3.1 分配策略

- 使用项目统一的内存分配器（`UVHTTP_MALLOC`, `UVHTTP_FREE`）
- 所有分配的内存必须检查返回值
- 资源释放必须在所有错误路径中处理

### 3.2 内存安全

```c
/* 正确的内存分配 */
char* buffer = (char*)UVHTTP_MALLOC(size);
if (!buffer) {
    return UVHTTP_ERROR_MEMORY;
}

/* 使用后必须释放 */
UVHTTP_FREE(buffer);
buffer = NULL; /* 避免悬空指针 */
```

## 4. 测试规范

### 4.1 测试类型

- **单元测试**：测试单个函数
- **集成测试**：测试模块间交互
- **压力测试**：测试性能和稳定性
- **安全测试**：测试边界条件

### 4.2 测试命名

```text
test_<module>_<feature>.c
例如：
- test_request_init.c
- test_websocket_integration.c
- test_server_stress.c
```

### 4.3 测试覆盖率

- 目标覆盖率：80%
- 核心模块：90%+
- 错误处理路径：100%

## 5. 提交规范

### 5.1 提交消息格式

```text
<type>(<scope>): <subject>

<body>

<footer>
```

### 5.2 类型说明

- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

### 5.3 提交示例

```text
feat(websocket): 添加 WebSocket 连接管理

- 实现连接池机制
- 添加自动重连功能
- 优化内存使用

Closes #123
```

## 6. 代码审查

### 6.1 审查清单

- [ ] 代码符合项目规范
- [ ] 错误处理完整
- [ ] 内存管理安全
- [ ] 测试覆盖充分
- [ ] 文档更新及时
- [ ] 性能影响可接受

### 6.2 审查流程

1. 创建 Pull Request
2. 至少一人审查
3. 通过所有测试
4. 合并到主分支

## 7. 版本管理

### 7.1 版本号规范

使用语义化版本号：`MAJOR.MINOR.PATCH`

- `MAJOR`：不兼容的 API 修改
- `MINOR`：向下兼容的功能性新增
- `PATCH`：向下兼容的问题修正

### 7.2 分支策略

- `main`：主分支，稳定版本
- `develop`：开发分支
- `feature/<name>`：功能分支
- `hotfix/<name>`：紧急修复分支

## 8. 性能要求

### 8.1 性能指标

- API 调用：< 10μs
- 内存分配：< 100ns
- 字符串操作：< 50ns
- 错误处理：< 1μs

### 8.2 性能测试

- 使用基准测试验证性能
- 定期运行性能回归测试
- 记录性能指标变化

## 9. 安全规范

### 9.1 安全原则

- 输入验证：所有外部输入必须验证
- 缓冲区保护：防止缓冲区溢出
- 资源限制：限制资源使用量
- 错误信息：不泄露敏感信息

### 9.2 安全检查点

```c
/* 输入验证 */
if (!input || length > MAX_LENGTH) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* 边界检查 */
if (index >= array_size) {
    return UVHTTP_ERROR_OUT_OF_BOUNDS;
}
```

## 10. 文档规范

### 10.1 代码文档

- 公共 API 必须有文档
- 复杂算法需要注释
- 魔数需要常量定义

### 10.2 API 文档

- 使用 Doxygen 格式
- 包含使用示例
- 说明错误情况

## 11. 构建规范

### 11.1 构建系统

- 使用 CMake 作为主要构建系统
- 支持 Debug 和 Release 配置
- 启用所有警告

### 11.2 编译选项

```cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Werror)
```

## 12. 依赖管理

### 12.1 第三方库

- 使用稳定版本
- 文档化依赖关系

### 12.2 子模块

- 使用 Git 子模块管理和下载依赖
- 固定到特定版本
- 定期更新安全补丁

## 13. 发布流程

### 13.1 发布检查

- [ ] 所有测试通过
- [ ] 文档更新
- [ ] 版本号更新
- [ ] 变更日志更新
- [ ] 性能测试通过

### 13.2 发布步骤

1. 更新版本号
2. 更新 CHANGELOG
3. 创建 Git 标签
4. 构建发布包
5. 发布到仓库

## 14. 故障排除

### 14.1 调试工具

- GDB：核心转储分析
- Valgrind：内存泄漏检测
- AddressSanitizer：运行时错误检测
- 静态分析：cppcheck

### 14.2 常见问题

- 内存泄漏：使用 RAII 模式
- 竞态条件：使用单线程模型避免竞争，并确保在多线程环境中多实例无竞争
- 性能问题：使用性能分析工具

## 15. 持续改进

### 15.1 代码质量

- 定期代码审查
- 自动化测试
- 静态分析
- 性能监控

### 15.2 技术债务

- 记录技术债务
- 制定偿还计划
- 定期重构

---

本规范文档将随着项目发展持续更新。所有贡献者都应遵循这些规范以确保代码质量和项目的长期可维护性。
