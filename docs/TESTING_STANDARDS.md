# UVHTTP 测试规范

## 概述

本文档定义了 UVHTTP 项目的测试规范，包括测试代码的组织、编写、命名和维护标准。

## 目录

1. [测试类型](#测试类型)
2. [测试框架](#测试框架)
3. [测试文件组织](#测试文件组织)
4. [测试命名规范](#测试命名规范)
5. [测试编写规范](#测试编写规范)
6. [断言使用规范](#断言使用规范)
7. [测试覆盖率要求](#测试覆盖率要求)
8. [测试文档要求](#测试文档要求)
9. [CI/CD 集成](#cicd-集成)
10. [测试维护](#测试维护)

## 测试类型

### 单元测试 (Unit Tests)

- **位置**: `test/unit/`
- **目的**: 测试单个函数、模块或类的功能
- **特点**:
  - 快速执行（毫秒级）
  - 独立性：不依赖外部资源
  - 可重复：每次执行结果一致
  - 自动化：可集成到 CI/CD

### 集成测试 (Integration Tests)

- **位置**: `test/integration/`
- **目的**: 测试多个模块之间的交互
- **特点**:
  - 中等执行时间（秒级）
  - 可能依赖外部资源（文件、网络）
  - 测试真实场景下的模块协作
  - 可自动化

### 性能测试 (Performance Tests)

- **位置**: `test/performance/`
- **目的**: 测试系统性能指标
- **特点**:
  - 执行时间较长（秒到分钟级）
  - 测试吞吐量、延迟、资源使用
  - 需要专门的测试工具
  - 通常不在每次构建时运行

## 测试框架

### C++ 测试（推荐）

**框架**: Google Test (gtest)

**优势**:
- 功能强大，断言丰富
- 跨平台支持
- 社区活跃，文档完善
- 与 CMake 集成良好

**使用场景**:
- 新编写的测试
- 需要复杂断言的测试
- 需要测试套件组织的测试

### C 测试（兼容）

**框架**: 标准 assert 宏

**优势**:
- 无外部依赖
- 简单直接
- 与现有代码兼容

**使用场景**:
- 现有的测试代码
- 简单的测试场景
- 不需要复杂断言的测试

## 测试文件组织

### 目录结构

```
test/
├── unit/                   # 单元测试
│   ├── test_*.cpp          # C++ 测试（使用 GTest）
│   ├── test_*.c            # C 测试（使用 assert）
│   └── simple_test.cpp     # GTest 示例测试
├── integration/            # 集成测试
│   ├── test_*.c            # C 测试
│   └── websocket_test.html # WebSocket 测试页面
├── performance/            # 性能测试
│   ├── test_*.c            # C 测试
│   └── performance_*.c      # 性能测试
├── config/                 # 配置文件
│   ├── config_*.conf
│   └── *.md
├── scripts/                # 测试脚本
│   └── *.sh
├── results/                # 测试结果（不提交）
└── CMakeLists.txt          # 测试 CMake 配置
```

### 文件命名规范

**单元测试**:
- C++ 测试: `test_<module>.cpp`
- C 测试: `test_<module>.c`
- 示例: `test_allocator.cpp`, `test_utils.c`

**集成测试**:
- 格式: `test_<feature>.c`
- 示例: `test_simple.c`, `test_route.c`

**性能测试**:
- 格式: `performance_<feature>.c`
- 示例: `performance_allocator.c`

## 测试命名规范

### 测试套件命名

**格式**: `<Module>Test`

**示例**:
- `UvhttpUtilsTest`
- `UvhttpAllocatorTest`
- `UvhttpValidationTest`

### 测试用例命名

**格式**: `CamelCase`，描述性强

**示例**:
- `SafeStrncpyNormal`
- `SafeStrncpyOverflow`
- `ValidateUrlValid`
- `NullFree`

### 测试函数命名（C 测试）

**格式**: `test_<feature>_<scenario>`

**示例**:
- `test_allocator_basic`
- `test_allocator_calloc`
- `test_manager_create_normal`

## 测试编写规范

### C++ 测试（GTest）

```cpp
#include <gtest/gtest.h>
#include "uvhttp_utils.h"

// 测试套件
TEST(UvhttpUtilsTest, SafeStrncpyNormal) {
    // Arrange
    char dest[10];
    
    // Act
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    
    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "hello");
}
```

### C 测试（assert）

```c
#include "uvhttp_utils.h"
#include <stdio.h>
#include <assert.h>

void test_safe_strncpy_normal(void) {
    // Arrange
    char dest[10];
    
    // Act
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    
    // Assert
    assert(result == 0);
    assert(strcmp(dest, "hello") == 0);
}
```

### AAA 模式

所有测试应遵循 **Arrange-Act-Assert** 模式：

1. **Arrange**（准备）：设置测试环境，初始化变量
2. **Act**（执行）：调用被测试的函数
3. **Assert**（断言）：验证结果

## 断言使用规范

### GTest 断言

| 断言类型 | 说明 | 失败行为 |
|---------|------|---------|
| `EXPECT_EQ(val1, val2)` | 期望相等 | 继续测试 |
| `ASSERT_EQ(val1, val2)` | 断言相等 | 停止测试 |
| `EXPECT_NE(val1, val2)` | 期望不等 | 继续测试 |
| `ASSERT_NE(val1, val2)` | 断言不等 | 停止测试 |
| `EXPECT_STREQ(str1, str2)` | 期望字符串相等 | 继续测试 |
| `ASSERT_STREQ(str1, str2)` | 断言字符串相等 | 停止测试 |
| `EXPECT_GT(val1, val2)` | 期望大于 | 继续测试 |
| `EXPECT_LT(val1, val2)` | 期望小于 | 继续测试 |
| `EXPECT_TRUE(condition)` | 期望为真 | 继续测试 |
| `EXPECT_FALSE(condition)` | 期望为假 | 继续测试 |
| `EXPECT_PTR_EQ(ptr1, ptr2)` | 期望指针相等 | 继续测试 |
| `EXPECT_PTR_NE(ptr1, ptr2)` | 期望指针不等 | 继续测试 |
| `EXPECT_NULL(ptr)` | 期望为 NULL | 继续测试 |
| `EXPECT_NOTNULL(ptr)` | 期望不为 NULL | 继续测试 |

### 标准 assert

```c
assert(condition);           // 失败则停止程序
assert(ptr != NULL);        // 检查指针非空
assert(a == b);             // 检查相等
assert(strcmp(s1, s2) == 0); // 检查字符串相等
```

### 断言选择原则

1. **使用 EXPECT**：
   - 非关键的验证
   - 可以继续执行后续测试
   - 需要收集多个失败信息

2. **使用 ASSERT**：
   - 关键的前置条件
   - 失败后无法继续测试
   - 防止空指针解引用

3. **标准 assert**：
   - C 测试代码
   - 简单的验证场景
   - 不需要复杂断言时

## 测试覆盖率要求

### 目标覆盖率

- **整体覆盖率**: ≥ 80%
- **核心模块覆盖率**: ≥ 90%
- **关键路径覆盖率**: 100%

### 覆盖率类型

- **行覆盖率**: 每行代码至少被执行一次
- **分支覆盖率**: 每个 if/else 分支至少被执行一次
- **函数覆盖率**: 每个函数至少被调用一次

### 覆盖率检查

```bash
# 生成覆盖率报告
cmake -DENABLE_COVERAGE=ON ..
make
./run_tests.sh --detailed

# 查看覆盖率报告
lcov --capture --directory build --output-file coverage.info
lcov --report coverage.info
```

## 测试文档要求

### 测试文件头部注释

每个测试文件应包含：

```c
/**
 * @file test_allocator.c
 * @brief UVHTTP 统一内存分配器测试
 * 
 * 测试内容：
 * - 基本分配和释放
 * - calloc 分配
 * - realloc 重新分配
 * - 边界条件处理
 * - 大内存分配
 * 
 * 测试覆盖率目标: 95%
 * 依赖: uvhttp_allocator.h, test_memory_helpers.c
 */
```

### 测试函数注释

复杂的测试函数应添加注释：

```c
/**
 * @brief 测试中等文件分块传输
 * 
 * 创建 5MB 测试文件，验证文件大小和内容
 * 
 * @note 此测试会创建临时文件，测试完成后自动删除
 */
static void test_medium_file_chunked_transfer(void) {
    // ...
}
```

## CI/CD 集成

### 自动化测试

所有测试必须集成到 CI/CD 系统：

```yaml
# .github/workflows/ci.yml
- name: Run unit tests
  run: make test

- name: Run integration tests
  run: ctest -R integration

- name: Generate coverage report
  run: ./run_tests.sh --detailed
```

### 测试失败处理

- 单元测试失败：阻止合并
- 集成测试失败：阻止合并
- 性能测试失败：发出警告，不阻止合并

## 测试维护

### 测试更新原则

1. **添加新功能时**：
   - 同时添加对应的测试
   - 确保测试覆盖率不下降

2. **修复 Bug 时**：
   - 先添加失败的测试用例
   - 修复 Bug 使测试通过
   - 防止回归

3. **重构代码时**：
   - 更新相关测试
   - 确保测试仍然通过
   - 保持测试覆盖率

### 测试清理

- 移除已废弃功能的测试
- 更新过时的测试用例
- 优化慢速测试

## 最佳实践

### 1. 测试独立性

每个测试应该独立运行，不依赖其他测试的执行顺序或状态。

### 2. 测试可重复性

测试结果应该可以重复，不依赖于外部环境或时间。

### 3. 测试快速性

单元测试应该快速执行（< 1 秒），集成测试应该尽量快（< 10 秒）。

### 4. 测试清晰性

测试名称和断言应该清晰表达测试意图。

### 5. 测试完整性

测试应该覆盖正常情况、边界情况和错误情况。

## 禁止事项

1. ❌ 不要在测试中使用全局变量
2. ❌ 不要在测试中使用硬编码的路径
3. ❌ 不要在测试中使用 sleep 等待
4. ❌ 不要在测试中依赖网络资源
5. ❌ 不要在测试中修改源代码
6. ❌ 不要在测试中忽略编译警告

## 示例

### GTest 示例

```cpp
#include <gtest/gtest.h>
#include "uvhttp_utils.h"

TEST(UvhttpUtilsTest, SafeStrncpyNormal) {
    char dest[10];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
}

TEST(UvhttpUtilsTest, SafeStrncpyOverflow) {
    char dest[5];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "123456789", sizeof(dest)), 0);
    EXPECT_LT(strlen(dest), sizeof(dest));
}
```

### C 测试示例

```c
#include "uvhttp_utils.h"
#include <stdio.h>
#include <assert.h>

void test_safe_strncpy_normal(void) {
    char dest[10];
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, "hello") == 0);
}

void test_safe_strncpy_overflow(void) {
    char dest[5];
    int result = uvhttp_safe_strncpy(dest, "123456789", sizeof(dest));
    assert(result == 0);
    assert(strlen(dest) < sizeof(dest));
}

int main(void) {
    test_safe_strncpy_normal();
    printf("✓ test_safe_strncpy_normal: PASSED\n");
    
    test_safe_strncpy_overflow();
    printf("✓ test_safe_strncpy_overflow: PASSED\n");
    
    return 0;
}
```

## 参考资源

- [Google Test 文档](https://google.github.io/googletest/)
- [UVHTTP 架构文档](../docs/ARCHITECTURE.md)
- [UVHTTP 开发指南](../docs/DEVELOPER_GUIDE.md)

## 版本历史

- v1.0 (2026-01-15): 初始版本，定义基本测试规范