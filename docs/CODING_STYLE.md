# UVHTTP 代码风格指南

## 概述

本文档定义了UVHTTP项目的代码风格和注释规范，确保代码的一致性和可读性。

## 代码风格

- **标准**：uvhttp 库源代码使用 C11 标准, 测试代码使用 C++17，实例代码包含 C 和 CPP 两个版本
- **代码风格**：uvhttp 库源代码使用 Linux 内核代码风格，测试代码和实例代码使用 google cpp 风格

## 注释风格

### 1. 文件头注释

每个源文件应包含以下格式的文件头注释：

```c
/* 
 * UVHTTP 模块名称
 * 
 * 模块功能描述
 * 
 * 作者信息（可选）
 * 创建时间（可选）
 */
```

### 2. 函数注释

所有公共API函数必须使用以下格式的注释：

```c
/**
 * 函数功能简述
 * 
 * 详细描述函数的功能、用途和行为
 * 
 * @param param1 参数1描述
 * @param param2 参数2描述
 * @return 返回值描述
 * @note 特殊说明或注意事项
 */
```

### 3. 内联注释

对于复杂的代码逻辑，使用内联注释：

```c
/* 单行注释：解释当前代码行的目的 */

/* 
 * 多行注释：
 * 解释复杂算法或逻辑
 * 可以跨越多行
 */
```

### 4. TODO和FIXME标记

使用标准格式标记待办事项：

```c
/* TODO: 添加错误处理 */
/* FIXME: 修复内存泄漏问题 */
```

## 命名规范

### 1. 函数命名

- 使用 `uvhttp_module_action` 格式
- 全部小写，单词间用下划线分隔
- 动词开头，描述函数功能

```c
uvhttp_server_new()
uvhttp_request_get_header()
uvhttp_response_send()
```

### 2. 变量命名

- 局部变量：小写，下划线分隔
- 全局变量：`g_` 前缀
- 常量：全大写，下划线分隔

```c
int connection_count;
static uvhttp_server_t* g_server;
#define MAX_BUFFER_SIZE 1024
```

### 3. 类型命名

- 结构体：`uvhttp_name_t`
- 枚举：`uvhttp_category_name`
- 联合体：`uvhttp_name_u`

```c
typedef struct uvhttp_server uvhttp_server_t;
typedef enum uvhttp_method uvhttp_method_t;
```

## 代码格式

### 1. 缩进

- 使用4个空格缩进，不使用制表符
- 大括号对齐风格：K&R风格

```c
if (condition) {
    /* 代码块 */
} else {
    /* 代码块 */
}
```

### 2. 空格使用

- 操作符前后加空格
- 逗号后加空格
- 函数名和括号间不加空格

```c
int result = a + b;
func(arg1, arg2);
```

### 3. 行长度

- 每行不超过120个字符
- 超长时合理换行

## 错误处理规范

### 1. 返回值检查

所有可能失败的函数调用都必须检查返回值：

```c
int result = uvhttp_server_new(loop);
if (result != UVHTTP_OK) {
    /* 错误处理 */
}
```

### 2. 错误日志

使用统一的错误日志函数：

```c
uvhttp_log_safe_error(error_code, "context", "message");
```

## 内存管理规范

### 1. 分配和释放

- 使用统一的内存分配函数
- 每个分配都必须有对应的释放
- 检查分配是否成功

```c
void* ptr = uvhttp_malloc(size);
if (!ptr) {
    return UVHTTP_ERROR_MEMORY;
}
/* 使用ptr */
uvhttp_free(ptr);
```

### 2. 资源清理

使用统一的清理函数：

```c
uvhttp_safe_free((void**)&ptr, NULL);
```

## 安全编码规范

### 1. 输入验证

所有外部输入都必须验证：

```c
if (!uvhttp_validate_string_length(input, min, max)) {
    return UVHTTP_ERROR_INVALID_INPUT;
}
```

### 2. 缓冲区操作

防止缓冲区溢出：

```c
strncpy(buffer, source, sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';
```

## 文档规范

### 1. API文档

所有公共API必须在头文件中有完整文档

### 2. 内部文档

复杂算法和设计决策必须有详细注释

## 代码审查清单

- [ ] 所有函数都有适当注释
- [ ] 错误处理完整
- [ ] 内存管理安全
- [ ] 输入验证充分
- [ ] 命名规范一致
- [ ] 代码格式正确
- [ ] 无硬编码魔法数字
- [ ] 无编译警告

## 工具配置

### 1. 编辑器配置

```editorconfig
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

[*.c]
indent_style = space
indent_size = 4
```

### 2. 代码检查工具

使用以下工具确保代码质量：
- `cppcheck` - 静态分析
- `clang-format` - 代码格式化
- `valgrind` - 内存检查

---

*本文档随项目发展持续更新*