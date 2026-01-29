# cmocka 集成指南

## 概述

cmocka 是一个专门用于 C 语言的单元测试 mock 框架，可以大大简化 libuv 函数的 mock 工作。

## cmocka 的优势

### 1. 自动化 Mock
- 无需手动编写 mock 函数实现
- cmocka 自动生成函数包装器
- 减少代码维护成本

### 2. 强大的验证功能
- 自动验证函数调用次数
- 参数验证和检查
- 返回值设置
- 回调函数验证

### 3. 易于使用
- 简洁的 API
- 与 GTest 完全兼容
- 清晰的错误报告

### 4. 零开销
- 只在测试时链接
- 生产环境无任何影响

## 迁移步骤

### 步骤 1: 添加 cmocka 依赖

```cmake
# 在 CMakeLists.txt 中添加
add_subdirectory(deps/cmocka)
target_link_libraries(your_test PRIVATE cmocka)
```

### 步骤 2: 创建 Mock 函数

```c
#include <cmocka.h>

/* 声明要 mock 的函数 */
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle);

/* 实现 mock 函数 */
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    (void)loop;
    (void)handle;
    return mock_type(int);  /* 从测试用例获取返回值 */
}
```

### 步骤 3: 编写测试用例

```c
static void test_uv_tcp_init_success(void** state) {
    (void)state;
    
    /* 设置返回值 */
    will_return(__wrap_uv_tcp_init, 0);
    
    /* 运行测试 */
    uv_loop_t loop;
    uv_tcp_t handle;
    int result = uv_tcp_init(&loop, &handle);
    
    /* 验证结果 */
    assert_int_equal(result, 0);
}
```

### 步骤 4: 验证调用次数

```c
static void test_multiple_calls(void** state) {
    (void)state;
    
    /* 设置返回值（每次调用都会按顺序返回） */
    will_return(__wrap_uv_tcp_init, 0);
    will_return(__wrap_uv_tcp_init, 0);
    
    /* 运行测试 */
    uv_tcp_init(&loop, &handle1);
    uv_tcp_init(&loop, &handle2);
    
    /* cmocka 会自动验证调用次数 */
}
```

### 步骤 5: 验证参数

```c
static void test_parameter_validation(void** state) {
    (void)state;
    
    /* 期望参数值 */
    expect_value(__wrap_uv_listen, backlog, 128);
    
    /* 运行测试 */
    uv_listen((uv_stream_t*)&server, 128, callback);
    
    /* cmocka 会自动验证参数 */
}
```

## 与当前方案对比

### 当前方案（手动 libuv mock 库）

```c
/* 需要手动实现 */
void libuv_mock_set_uv_tcp_init_result(int result) {
    g_mock_state->uv_tcp_init_result = result;
}

int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    return g_mock_state->uv_tcp_init_result;
}

/* 需要手动验证调用 */
size_t count;
libuv_mock_get_call_count("uv_tcp_init", &count);
assert_int_equal(count, 1);
```

### cmocka 方案

```c
/* 自动生成 mock 函数 */
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    return mock_type(int);
}

/* 自动验证调用 */
will_return(__wrap_uv_tcp_init, 0);
uv_tcp_init(&loop, &handle);
/* cmocka 自动验证调用次数 */
```

## 最佳实践

### 1. 使用 `__wrap_` 前缀
```c
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle);
```

### 2. 在测试文件中实现 mock 函数
```c
/* test/test_uv_tcp.c */
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    return mock_type(int);
}
```

### 3. 使用链接器 wrap
```cmake
target_link_options(your_test PRIVATE
    -Wl,--wrap=uv_tcp_init
    -Wl,--wrap=uv_tcp_bind
    ...
)
```

### 4. 清理 mock 状态
```c
static int setup(void** state) {
    /* 初始化测试环境 */
    return 0;
}

static int teardown(void** state) {
    /* 清理测试环境 */
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_case, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

## 高级用法

### 1. 自定义参数检查
```c
static int check_sockaddr_ptr(const LargestIntegralType value, const void* check_data) {
    struct sockaddr* addr = (struct sockaddr*)value;
    assert_non_null(addr);
    return 1;
}

expect_check(__wrap_uv_tcp_bind, addr, check_sockaddr_ptr, NULL);
```

### 2. 模拟回调
```c
/* 在 mock 函数中触发回调 */
int __wrap_uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb) {
    check_expected(backlog);
    
    /* 获取回调函数 */
    uv_connection_cb callback = mock_ptr_type(uv_connection_cb);
    
    /* 触发回调 */
    if (callback) {
        callback(stream, 0);
    }
    
    return mock_type(int);
}

/* 在测试中设置回调 */
will_return(__wrap_uv_listen, my_connection_callback);
will_return(__wrap_uv_listen, 0);
```

### 3. 部分参数验证
```c
/* 只验证部分参数 */
expect_value(__wrap_uv_tcp_bind, flags, 0);
/* 其他参数不验证 */
```

## 迁移检查清单

- [ ] 添加 cmocka 依赖到 CMakeLists.txt
- [ ] 创建 mock 函数包装器
- [ ] 更新测试用例使用 cmocka API
- [ ] 移除手动 mock 库代码
- [ ] 验证所有测试通过
- [ ] 更新文档

## 注意事项

1. **链接顺序**：确保 cmocka 在测试目标之前链接
2. **函数签名**：mock 函数签名必须与原函数完全一致
3. **返回值**：使用 `mock_type()` 获取返回值
4. **清理**：使用 `setup/teardown` 清理测试状态
5. **兼容性**：cmocka 与 GTest 可以混合使用

## 参考资料

- cmocka 官方文档: https://cmocka.org/
- cmocka API 参考: https://api.cmocka.org/
- 示例代码: `test/mock/cmocka_example.c`