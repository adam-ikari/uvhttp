# 前向声明和循环引用规范

本文档定义了 UVHTTP 项目的前向声明和循环引用规范，确保代码的可维护性和编译效率。

## 概述

前向声明（Forward Declaration）和循环引用（Circular Dependency）是 C/C++ 项目中常见的设计问题。正确使用前向声明可以避免不必要的包含，减少编译时间，提高代码的可维护性。

## 前向声明规范

### 何时使用前向声明

**应该使用前向声明的情况**：

1. **在头文件中只需要指针或引用**
   ```c
   // ✅ 正确
   typedef struct uvhttp_server uvhttp_server_t;
   
   struct uvhttp_connection {
       struct uvhttp_server* server;  // 只需要指针
   };
   ```

2. **避免循环引用**
   ```c
   // uvhttp_server.h
   typedef struct uvhttp_connection uvhttp_connection_t;
   
   // uvhttp_connection.h
   typedef struct uvhttp_server uvhttp_server_t;
   ```

3. **在函数声明中使用指针参数**
   ```c
   // ✅ 正确
   uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                        uvhttp_connection_t** conn);
   ```

**不应该使用前向声明的情况**：

1. **需要访问结构体成员**
   ```c
   // ❌ 错误
   typedef struct uvhttp_request uvhttp_request_t;
   
   void func(uvhttp_request_t* req) {
       req->method = UVHTTP_GET;  // 需要访问成员
   }
   
   // ✅ 正确
   #include "uvhttp_request.h"
   ```

2. **需要调用结构体方法**
   ```c
   // ❌ 错误
   typedef struct uvhttp_response uvhttp_response_t;
   
   void func(uvhttp_response_t* resp) {
       uvhttp_response_set_status(resp, 200);  // 需要调用方法
   }
   
   // ✅ 正确
   #include "uvhttp_response.h"
   ```

3. **需要完整类型定义**
   ```c
   // ❌ 错误
   typedef struct uvhttp_request uvhttp_request_t;
   
   void func() {
       uvhttp_request_t req;  // 需要完整类型
   }
   
   // ✅ 正确
   #include "uvhttp_request.h"
   ```

### 前向声明语法

```c
// 基本类型
typedef struct uvhttp_server uvhttp_server_t;

// 指针类型
typedef struct uvhttp_server* uvhttp_server_ptr_t;

// 函数指针类型
typedef int (*uvhttp_handler_t)(void* context);

// 结构体内部
struct uvhttp_connection {
    struct uvhttp_server* server;
    uvhttp_request_t* request;
    uvhttp_response_t* response;
};
```

## 循环引用规范

### 循环引用的类型

#### 类型 1：A 包含 B，B 包含 A

```c
// uvhttp_server.h
#include "uvhttp_connection.h"  // ❌ 循环引用

// uvhttp_connection.h
#include "uvhttp_server.h"  // ❌ 循环引用
```

#### 类型 2：A 包含 B，B 包含 C，C 包含 A

```c
// uvhttp_server.h
#include "uvhttp_connection.h"

// uvhttp_connection.h
#include "uvhttp_request.h"

// uvhttp_request.h
#include "uvhttp_server.h"  // ❌ 循环引用
```

### 解决循环引用的方法

#### 方法 1：使用前向声明（推荐）

```c
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;  // ✅ 前向声明

struct uvhttp_server {
    uvhttp_connection_t* connections;
};

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t;  // ✅ 前向声明

struct uvhttp_connection {
    struct uvhttp_server* server;
};
```

#### 方法 2：提取公共接口到独立头文件

```c
// uvhttp_common.h（公共接口）
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;

// uvhttp_server.h
#include "uvhttp_common.h"  // ✅ 使用公共接口

// uvhttp_connection.h
#include "uvhttp_common.h"  // ✅ 使用公共接口
```

#### 方法 3：使用 void* 指针（不推荐）

```c
// ❌ 不推荐：失去类型安全
struct uvhttp_connection {
    void* server;  // 失去类型信息
};
```

## UVHTTP 项目的实际应用

### 已修复的循环引用

#### 1. uvhttp_server.h 和 uvhttp_connection.h

**修复前**：
```c
// uvhttp_server.h
#include "uvhttp_connection.h"  // ❌ 循环引用

// uvhttp_connection.h
#include "uvhttp_server.h"  // ❌ 循环引用
```

**修复后**：
```c
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;  // ✅ 前向声明

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t;  // ✅ 前向声明
```

#### 2. uvhttp_response.h

**修复前**：
```c
#include "uvhttp_connection.h"  // ❌ 可能导致循环引用
```

**修复后**：
```c
typedef struct uvhttp_connection uvhttp_connection_t;  // ✅ 前向声明
```

### 当前的前向声明使用情况

| 头文件 | 前向声明类型 | 用途 |
|--------|-------------|------|
| uvhttp_common.h | uvhttp_request_t, uvhttp_response_t | 定义请求处理器类型 |
| uvhttp_response.h | uvhttp_connection_t, uvhttp_response_t | 避免循环引用 |
| uvhttp_utils.h | uvhttp_response_t | 函数参数 |
| uvhttp_router.h | uvhttp_response_t | 函数参数 |
| uvhttp_server.h | uvhttp_request_t, uvhttp_response_t, uvhttp_connection_t, uvhttp_router_t, uvhttp_connection_t | 避免循环引用 |
| uvhttp_connection.h | uvhttp_connection_t, uvhttp_server_t | 避免循环引用 |

## 最佳实践

### 1. 头文件包含顺序

```c
// 1. 对应的头文件（如果有）
#include "uvhttp_server.h"

// 2. UVHTTP 项目头文件（按字母顺序）
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"

// 3. 项目内部其他头文件（按字母顺序）
#include "uthash.h"

// 4. 标准库头文件（按字母顺序）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 5. 第三方库头文件（按字母顺序）
#include <uv.h>

// 6. 前向声明（按字母顺序）
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 7. 条件编译包含
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

### 2. 在 .c 文件中包含完整定义

```c
// ✅ 正确：在 .c 文件中包含完整定义
#include "uvhttp_server.h"
#include "uvhttp_connection.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

// 在 .c 文件中可以访问所有成员
void some_function() {
    uvhttp_server_t* server = ...;
    uvhttp_connection_t* conn = ...;
    conn->server = server;  // ✅ 可以访问成员
}
```

### 3. 在 .h 文件中使用前向声明

```c
// ✅ 正确：在 .h 文件中使用前向声明
typedef struct uvhttp_server uvhttp_server_t;
typedef struct uvhttp_connection uvhttp_connection_t;

struct uvhttp_connection {
    struct uvhttp_server* server;  // ✅ 只需要指针
    uvhttp_request_t* request;
    uvhttp_response_t* response;
};
```

### 4. 使用公共接口头文件

```c
// uvhttp_common.h（公共接口）
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef int (*uvhttp_request_handler_t)(uvhttp_request_t*, uvhttp_response_t*);

// 其他头文件
#include "uvhttp_common.h"  // ✅ 使用公共接口
```

## 检测工具

### 1. 编译器警告

```bash
# GCC
gcc -Wmissing-include-dirs -Werror

# Clang
clang -Wmissing-include-dirs -Werror
```

### 2. 静态分析工具

```bash
# cppcheck
cppcheck --enable=missingInclude src/ include/

# clang-tidy
clang-tidy src/*.c include/*.h
```

### 3. 依赖分析工具

```bash
# include-what-you-use
include-what-you-use src/*.c include/*.h
```

## 常见错误

### 错误 1：在头文件中访问成员

```c
// ❌ 错误
typedef struct uvhttp_request uvhttp_request_t;

void func(uvhttp_request_t* req) {
    req->method = UVHTTP_GET;  // ❌ 需要完整定义
}

// ✅ 正确
#include "uvhttp_request.h"

void func(uvhttp_request_t* req) {
    req->method = UVHTTP_GET;  // ✅ 可以访问成员
}
```

### 错误 2：循环引用导致编译错误

```c
// ❌ 错误：循环引用
// uvhttp_server.h
#include "uvhttp_connection.h"

// uvhttp_connection.h
#include "uvhttp_server.h"

// ✅ 正确：使用前向声明
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t
```

### 错误 3：过度使用前向声明

```c
// ❌ 错误：过度使用前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;
typedef struct uvhttp_router uvhttp_router_t;

void func() {
    uvhttp_request_t req;  // ❌ 需要完整定义
    uvhttp_response_t resp;
    uvhttp_connection_t conn;
    // ...
}

// ✅ 正确：在 .c 文件中包含完整定义
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_connection.h"

void func() {
    uvhttp_request_t req;  // ✅ 可以使用完整类型
    uvhttp_response_t resp;
    uvhttp_connection_t conn;
    // ...
}
```

## 性能影响

### 编译时间

- **使用前向声明**：减少不必要的包含，加快编译
- **循环引用**：增加编译时间，可能导致无限递归

### 二进制大小

- **使用前向声明**：不影响二进制大小
- **循环引用**：不影响二进制大小（只影响编译）

### 运行时性能

- **使用前向声明**：无影响
- **循环引用**：无影响

## 参考资源

- [Google C++ Style Guide - Forward Declarations](https://google.github.io/styleguide/cppguide.html#Forward_Declarations)
- [C++ FAQ - What are forward declarations?](https://isocpp.org/wiki/faq/forward-declarations)
- [Include Guards and Forward Declarations](https://en.cppreference.com/w/cpp/header/include)

---

**最后更新**: 2026-01-29
**维护者**: UVHTTP 开发团队
**许可证**: MIT License