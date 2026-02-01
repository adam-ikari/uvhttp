# 头文件包含顺序规范

本文档定义了 UVHTTP 项目的头文件包含顺序规范，确保代码一致性和可维护性。

## 规范概述

头文件包含顺序遵循以下原则：

1. **相关性优先**: 相关的头文件应该放在一起
2. **本地优先**: 项目内部头文件优先于系统头文件
3. **类型分组**: 按类型分组（本地、系统、第三方）
4. **字母顺序**: 同一组内按字母顺序排列

## 包含顺序

### 1. 对应的头文件（如果是 .c 文件）

每个 .c 文件应该首先包含对应的 .h 文件（如果有）。

```c
#include "uvhttp_server.h"
```

### 2. UVHTTP 项目头文件

所有以 `uvhttp_` 开头的项目头文件，按字母顺序排列。

```c
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"
```

### 3. 项目内部其他头文件

其他项目内部头文件（非 uvhttp_ 开头），按字母顺序排列。

```c
#include "uthash.h"
```

### 4. 标准库头文件

C 标准库头文件，按字母顺序排列。

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

### 5. 第三方库头文件

第三方库头文件，按字母顺序排列。

```c
#include <uv.h>
```

### 6. 条件编译包含

条件编译的头文件应该放在最后，按字母顺序排列。

```c
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif

#ifdef UVHTTP_FEATURE_TLS
#include "uvhttp_tls.h"
#endif
```

## 完整示例

### .c 文件示例

```c
/*
 * UVHTTP 服务器模块
 *
 * 提供HTTP服务器的核心功能
 */

#include "uvhttp_server.h"

// UVHTTP 项目头文件
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"

// 标准库头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 第三方库头文件
#include <uv.h>

// 条件编译包含
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

### .h 文件示例

```c
#ifndef UVHTTP_SERVER_H
#    define UVHTTP_SERVER_H

// UVHTTP 项目头文件
#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"

// 标准库头文件
#include <assert.h>

// 第三方库头文件
#include <uv.h>

// 项目内部其他头文件
#include "uthash.h"

// UVHTTP 项目头文件（依赖）
#include "uvhttp_features.h"

// 条件编译包含
#    if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#    endif

#endif // UVHTTP_SERVER_H
```

## 自动格式化

使用 `clang-format` 自动格式化包含顺序：

```bash
# 检查格式
make format-check

# 修复格式
make format-fix

# 格式化所有文件
make format-all
```

## 规则说明

### 1. 对应的头文件优先

- 每个 .c 文件应该首先包含对应的 .h 文件
- 这有助于验证头文件的独立性

### 2. UVHTTP 项目头文件分组

- 所有以 `uvhttp_` 开头的头文件放在一起
- 按字母顺序排列
- 便于查找和维护

### 3. 项目内部其他头文件

- 非 uvhttp_ 开头的项目头文件
- 如 `uthash.h`
- 按字母顺序排列

### 4. 标准库头文件

- C 标准库头文件（`<*.h>`）
- 按字母顺序排列
- 便于识别依赖关系

### 5. 第三方库头文件

- 第三方库头文件（`<*>`）
- 如 `<uv.h>`, `<mbedtls/*.h>`
- 按字母顺序排列

### 6. 条件编译包含

- 所有条件编译的包含放在最后
- 按字母顺序排列
- 便于阅读和维护

## 常见错误

### 错误 1: 系统头文件在前

❌ 错误：

```c
#include <stdio.h>
#include <stdlib.h>
#include "uvhttp_server.h"
#include "uvhttp_config.h"
```

✅ 正确：

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#include <stdio.h>
#include <stdlib.h>
```

### 错误 2: 未按字母顺序

❌ 错误：

```c
#include "uvhttp_utils.h"
#include "uvhttp_config.h"
#include "uvhttp_server.h"
```

✅ 正确：

```c
#include "uvhttp_config.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"
```

### 错误 3: 条件编译包含混在中间

❌ 错误：

```c
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
#include "uvhttp_config.h"
```

✅ 正确：

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

## 工具支持

### clang-format

项目使用 `clang-format` 自动格式化代码，包括头文件包含顺序。

配置文件：`.clang-format`

### Makefile 命令

```bash
# 检查格式
make format-check

# 修复格式
make format-fix

# 格式化所有文件
make format-all
```

## CI/CD 集成

在 CI/CD 工作流中可以添加格式检查：

```yaml
- name: Check code format
  run: |
    make format-check
```

## 参考资源

- [Google C++ Style Guide - Names and Order of Includes](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes)
- [clang-format - IncludeCategories](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#includecategories)

---

**最后更新**: 2026-01-29
**维护者**: UVHTTP 开发团队
**许可证**: MIT License