# 头文件包含顺序规范

## 规范概述

包含顺序遵循四条规则：

1. 相关的头文件放在一起。
2. 本地头文件排在系统头文件之前。
3. 头文件按类型分组：本地、系统、第三方。
4. 每组内部按字母顺序排列。

## 包含顺序

### 1. 对应的头文件（.c 文件）

.c 文件若有自己的头文件，则最先包含它。

```c
#include "uvhttp_server.h"
```

### 2. UVHTTP 项目头文件

项目头文件以 `uvhttp_` 开头，按字母顺序排列。

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

不以 `uvhttp_` 开头的内部头文件排在后面，按字母顺序排列。

```c
#include "uthash.h"
```

### 4. 标准库头文件

接下来是 C 标准库头文件，按字母顺序排列。

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

### 5. 第三方库头文件

然后是第三方库头文件，按字母顺序排列。

```c
#include <uv.h>
```

### 6. 条件编译包含

条件包含放在最后，按字母顺序排列。

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
 * 提供 HTTP 服务器的核心功能
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

`clang-format` 会自动排好包含顺序：

```bash
# 就地格式化文件
clang-format -i src/uvhttp_server.c

# 只检查，不写文件
clang-format --dry-run --Werror src/uvhttp_server.c
```

`just fmt` 格式化 `src/`、`include/`、`test/` 下的所有 C 源文件和头文件；`just check-style` 执行同样的检查，但不写文件。

## 规则说明

### 1. 对应的头文件优先

每个 .c 文件最先包含自己的头文件。这能验证头文件是自包含的。

### 2. UVHTTP 项目头文件分组

以 `uvhttp_` 开头的头文件组成一组，按字母顺序排列。分组便于查找和维护。

### 3. 项目内部其他头文件

不以 `uvhttp_` 开头的内部头文件单独成组，按字母顺序排列，例如 `uthash.h`。

### 4. 标准库头文件

C 标准库头文件（`<*.h>`）按字母顺序排列，文件依赖一目了然。

### 5. 第三方库头文件

第三方头文件（`<*>`）排在后面，例如 `<uv.h>` 和 `<mbedtls/*.h>`，按字母顺序排列。

### 6. 条件编译包含

条件包含放在最后，按字母顺序排列。不放在中间，无条件包含的列表更容易浏览。

## 常见错误

### 错误 1：系统头文件在前

错误：

```c
#include <stdio.h>
#include <stdlib.h>
#include "uvhttp_server.h"
#include "uvhttp_config.h"
```

正确：

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#include <stdio.h>
#include <stdlib.h>
```

### 错误 2：未按字母顺序

错误：

```c
#include "uvhttp_utils.h"
#include "uvhttp_config.h"
#include "uvhttp_server.h"
```

正确：

```c
#include "uvhttp_config.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"
```

### 错误 3：条件编译包含混在中间

错误：

```c
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
#include "uvhttp_config.h"
```

正确：

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

## 工具支持

### clang-format

项目用 `clang-format` 格式化代码，包含顺序也在其中。

配置文件：`.clang-format`

### Justfile 命令

```bash
# 格式化所有 C 源文件和头文件
just fmt

# 只检查风格，不写文件
just check-style
```

## CI/CD 集成

格式检查可以加入 CI/CD 工作流：

```yaml
- name: Check code format
  run: |
    clang-format --dry-run --Werror $(find src include test -name '*.c' -o -name '*.h')
```

## 参考资源

- [Google C++ Style Guide - Names and Order of Includes](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes)
- [clang-format - IncludeCategories](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#includecategories)

---

**最后更新**: 2026-01-29
**维护者**: UVHTTP 开发团队
**许可证**: MIT License
