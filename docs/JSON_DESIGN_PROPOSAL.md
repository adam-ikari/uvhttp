# UVHTTP JSON 序列化/反序列化设计方案

## 📋 设计目标

1. **易用性优先**：提供简单直观的 API，减少样板代码
2. **灵活性兼顾**：支持复杂场景和自定义需求
3. **性能优化**：零拷贝设计，内存高效
4. **类型安全**：编译时类型检查，运行时验证
5. **错误处理**：完善的错误报告和恢复机制

## 🏗️ 架构设计

### 三层架构

```
┌─────────────────────────────────────┐
│        高级 API (便利层)              │
│  - 链式构建器                       │
│  - 模板函数                         │
│  - 自动类型推断                     │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│        中级 API (功能层)              │
│  - 对象/数组操作                    │
│  - 类型转换                         │
│  - 路径表达式                       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│        低级 API (基础层)              │
│  - cJSON 封装                       │
│  - 内存管理                         │
│  - 错误处理                         │
└─────────────────────────────────────┘
```

## 🎯 核心设计

### 1. 类型系统

```c
// 增强的类型系统
typedef enum {
    UVJSON_NULL = 0,
    UVJSON_BOOL,
    UVJSON_INT,
    UVJSON_DOUBLE,
    UVJSON_STRING,
    UVJSON_ARRAY,
    UVJSON_OBJECT,
    UVJSON_BINARY,    // 新增：二进制数据
    UVJSON_DATETIME,  // 新增：日期时间
    UVJSON_UUID       // 新增：UUID
} uvjson_type_t;

// 值联合体，支持更多类型
typedef struct {
    uvjson_type_t type;
    union {
        bool bool_val;
        int64_t int_val;
        double double_val;
        struct {
            char* data;
            size_t len;
        } string_val;
        struct {
            uint8_t* data;
            size_t len;
        } binary_val;
        time_t datetime_val;
        struct {
            uint32_t high;
            uint32_t low;
        } uuid_val;
        struct uvjson_array* array_val;
        struct uvjson_object* object_val;
    } data;
} uvjson_value_t;
```

### 2. 高级 API - 链式构建器

```c
// 链式构建器设计
typedef struct uvjson_builder uvjson_builder_t;

// 创建构建器
uvjson_builder_t* uvjson_builder_create(void);
uvjson_builder_t* uvjson_builder_create_object(void);
uvjson_builder_t* uvjson_builder_create_array(void);

// 链式方法
uvjson_builder_t* uvjson_builder_set_string(uvjson_builder_t* builder, 
                                           const char* key, const char* value);
uvjson_builder_t* uvjson_builder_set_int(uvjson_builder_t* builder, 
                                         const char* key, int64_t value);
uvjson_builder_t* uvjson_builder_set_double(uvjson_builder_t* builder, 
                                           const char* key, double value);
uvjson_builder_t* uvjson_builder_set_bool(uvjson_builder_t* builder, 
                                          const char* key, bool value);
uvjson_builder_t* uvjson_builder_set_null(uvjson_builder_t* builder, 
                                          const char* key);

// 数组操作
uvjson_builder_t* uvjson_builder_add_string(uvjson_builder_t* builder, const char* value);
uvjson_builder_t* uvjson_builder_add_int(uvjson_builder_t* builder, int64_t value);
uvjson_builder_t* uvjson_builder_add_double(uvjson_builder_t* builder, double value);
uvjson_builder_t* uvjson_builder_add_bool(uvjson_builder_t* builder, bool value);
uvjson_builder_t* uvjson_builder_add_null(uvjson_builder_t* builder);

// 嵌套对象/数组
uvjson_builder_t* uvjson_builder_begin_object(uvjson_builder_t* builder, const char* key);
uvjson_builder_t* uvjson_builder_begin_array(uvjson_builder_t* builder, const char* key);
uvjson_builder_t* uvjson_builder_end(uvjson_builder_t* builder);

// 序列化
char* uvjson_builder_stringify(uvjson_builder_t* builder);
char* uvjson_builder_stringify_pretty(uvjson_builder_t* builder);
uvjson_value_t* uvjson_builder_build(uvjson_builder_t* builder);

// 清理
void uvjson_builder_free(uvjson_builder_t* builder);
```

### 3. 使用示例

#### **简单对象构建**
```c
uvjson_builder_t* builder = uvjson_builder_create_object();
uvjson_builder_set_string(builder, "name", "张三");
uvjson_builder_set_int(builder, "age", 25);
uvjson_builder_set_bool(builder, "active", true);

char* json_str = uvjson_builder_stringify(builder);
// {"name":"张三","age":25,"active":true}

uvjson_builder_free(builder);
free(json_str);
```

#### **复杂嵌套结构**
```c
uvjson_builder_t* builder = uvjson_builder_create_object();
uvjson_builder_set_string(builder, "user", "张三");
uvjson_builder_begin_array(builder, "tags")
    ->add_string(builder, "developer")
    ->add_string(builder, "golang")
    ->add_string(builder, "javascript")
    ->end(builder);

uvjson_builder_begin_object(builder, "profile")
    ->set_string(builder, "email", "zhangsan@example.com")
    ->set_int(builder, "score", 95)
    ->begin_array(builder, "skills")
        ->add_string(builder, "programming")
        ->add_string(builder, "design")
        ->end(builder)
    ->end(builder);

char* json_str = uvjson_builder_stringify_pretty(builder);
uvjson_builder_free(builder);
```

### 4. 模板函数（便利层）

```c
// HTTP 响应模板
char* uvjson_build_response(int status, const char* message, ...);
char* uvjson_build_error(const char* error, const char* details, ...);
char* uvjson_build_success(const char* message, ...);

// 数据结构模板
char* uvjson_build_pagination(int page, int limit, int total, uvjson_builder_t* data);
char* uvjson_build_list_result(int count, uvjson_builder_t* items);

// 使用示例
char* response = uvjson_build_response(200, "操作成功",
    "user", uvjson_builder_create_object()
        ->set_string("name", "张三")
        ->set_int("age", 25),
    "timestamp", time(NULL),
    NULL);
```

### 5. 反序列化设计

```c
// 路径表达式 API
typedef struct uvjson_path uvjson_path_t;

uvjson_path_t* uvjson_path_create(const char* path_expression);
uvjson_value_t* uvjson_get_value(uvjson_value_t* root, uvjson_path_t* path);
uvjson_value_t* uvjson_get_value_by_string(uvjson_value_t* root, const char* path);

// 类型安全的获取函数
bool uvjson_get_bool(uvjson_value_t* json, const char* path, bool default_val);
int64_t uvjson_get_int(uvjson_value_t* json, const char* path, int64_t default_val);
double uvjson_get_double(uvjson_value_t* json, const char* path, double default_val);
char* uvjson_get_string(uvjson_value_t* json, const char* path, const char* default_val);

// 批量提取
typedef struct {
    const char* path;
    uvjson_type_t expected_type;
    void* target;
    bool found;
} uvjson_field_t;

int uvjson_extract_fields(uvjson_value_t* json, uvjson_field_t* fields, size_t count);

// 使用示例
uvjson_value_t* json = uvjson_parse(json_string);

// 单个值获取
int64_t user_id = uvjson_get_int(json, "user.id", 0);
char* username = uvjson_get_string(json, "user.profile.name", "anonymous");

// 批量提取
int64_t id;
char* name;
bool active;
uvjson_field_t fields[] = {
    {"user.id", UVJSON_INT, &id},
    {"user.name", UVJSON_STRING, &name},
    {"user.active", UVJSON_BOOL, &active}
};
uvjson_extract_fields(json, fields, 3);
```

### 6. 错误处理

```c
// 错误类型
typedef enum {
    UVJSON_OK = 0,
    UVJSON_ERROR_INVALID_JSON = -1,
    UVJSON_ERROR_INVALID_PATH = -2,
    UVJSON_ERROR_TYPE_MISMATCH = -3,
    UVJSON_ERROR_OUT_OF_MEMORY = -4,
    UVJSON_ERROR_PARSE_ERROR = -5
} uvjson_error_t;

// 错误上下文
typedef struct {
    uvjson_error_t code;
    char message[256];
    char path[128];
    int line;
    int column;
} uvjson_error_context_t;

// 错误处理 API
uvjson_error_t uvjson_get_last_error(void);
const uvjson_error_context_t* uvjson_get_error_context(void);
void uvjson_clear_error(void);

// 使用示例
uvjson_value_t* json = uvjson_parse(json_string);
if (uvjson_get_last_error() != UVJSON_OK) {
    const uvjson_error_context_t* ctx = uvjson_get_error_context();
    fprintf(stderr, "JSON 解析错误: %s (行 %d, 列 %d)\n", 
            ctx->message, ctx->line, ctx->column);
}
```

### 7. 性能优化

```c
// 内存池
typedef struct uvjson_pool uvjson_pool_t;

uvjson_pool_t* uvjson_pool_create(size_t initial_size);
void uvjson_pool_destroy(uvjson_pool_t* pool);
void* uvjson_pool_alloc(uvjson_pool_t* pool, size_t size);
void uvjson_pool_reset(uvjson_pool_t* pool);

// 零拷贝解析
typedef struct {
    const char* data;
    size_t len;
} uvjson_string_view_t;

uvjson_value_t* uvjson_parse_view(uvjson_string_view_t view);

// 流式解析
typedef struct uvjson_parser uvjson_parser_t;

uvjson_parser_t* uvjson_parser_create(void);
uvjson_error_t uvjson_parser_feed(uvjson_parser_t* parser, const char* data, size_t len);
uvjson_value_t* uvjson_parser_finish(uvjson_parser_t* parser);
void uvjson_parser_destroy(uvjson_parser_t* parser);
```

### 8. 与 HTTP 集成

```c
// HTTP 响应构建器集成
typedef struct {
    uvhttp_response_t* response;
    uvjson_builder_t* builder;
} uvhttp_json_response_t;

uvhttp_json_response_t* uvhttp_json_response_create(uvhttp_response_t* response);
uvhttp_result_t uvhttp_json_response_send(uvhttp_json_response_t* json_resp);

// 使用示例
uvhttp_result_t handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_json_response_t* json_resp = uvhttp_json_response_create(res);
    
    uvhttp_json_response_set(json_resp, "status", "success")
        ->set_int(json_resp, "code", 200)
        ->set_string(json_resp, "message", "操作成功")
        ->begin_array(json_resp, "data")
            ->add_string(json_resp, "item1")
            ->add_string(json_resp, "item2")
            ->end(json_resp);
    
    return uvhttp_json_response_send(json_resp);
}
```

## 🚀 实施计划

### 阶段一：基础框架（2周）
- [ ] 实现基础类型系统
- [ ] 完成 cJSON 封装层
- [ ] 实现基本的序列化/反序列化
- [ ] 添加错误处理机制

### 阶段二：高级功能（3周）
- [ ] 实现链式构建器
- [ ] 添加路径表达式支持
- [ ] 实现模板函数
- [ ] 性能优化（内存池）

### 阶段三：集成与优化（2周）
- [ ] 与 HTTP 层集成
- [ ] 完善文档和示例
- [ ] 性能测试和调优
- [ ] 单元测试覆盖

## 📊 性能目标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 序列化速度 | > 100MB/s | 1MB JSON 对象 |
| 反序列化速度 | > 50MB/s | 1MB JSON 字符串 |
| 内存开销 | < 2x | 相比原始数据 |
| 错误恢复时间 | < 1ms | 解析错误处理 |

## 🧪 测试策略

### 单元测试
- 类型转换测试
- 边界条件测试
- 错误处理测试
- 内存泄漏测试

### 性能测试
- 大数据量测试
- 高并发测试
- 内存压力测试
- 对比测试（vs cJSON、json-c）

### 集成测试
- HTTP 响应构建测试
- 复杂嵌套结构测试
- 实际应用场景测试

## 📚 文档计划

1. **API 参考文档**：完整的函数文档
2. **使用指南**：从入门到高级
3. **最佳实践**：性能优化建议
4. **迁移指南**：从现有代码迁移
5. **示例集合**：常见场景示例

## 🎯 总结

这个设计方案通过三层架构实现了易用性和灵活性的平衡：
- **高级 API** 提供简单直观的接口
- **中级 API** 提供丰富的功能
- **低级 API** 提供最大的控制力

链式构建器和模板函数大大简化了 JSON 的创建，而路径表达式和类型安全的获取函数使反序列化更加便捷。性能优化和错误处理确保了生产环境的可用性。