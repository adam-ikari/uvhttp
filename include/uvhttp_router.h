#ifndef UVHTTP_ROUTER_H
#define UVHTTP_ROUTER_H

#include "uvhttp_common.h"
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"

#include <assert.h>
#include <stddef.h>

// Forward declarations
typedef struct uvhttp_response uvhttp_response_t;

// HTTP方法枚举 - 使用uvhttp_request.h中的定义

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 128
#define MAX_ROUTE_PATH_LEN 256
#define MAX_PARAMS 16
#define MAX_PARAM_NAME_LEN 64
#define MAX_PARAM_VALUE_LEN 256

// 路由参数
typedef struct {
    char name[MAX_PARAM_NAME_LEN];
    char value[MAX_PARAM_VALUE_LEN];
} uvhttp_param_t;

// 路由匹配结果
typedef struct {
    uvhttp_request_handler_t handler;
    uvhttp_param_t params[MAX_PARAMS];
    size_t param_count;
} uvhttp_route_match_t;

// 路由节点
typedef struct uvhttp_route_node {
    char segment[64];  // 路径段
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    struct uvhttp_route_node* children[UVHTTP_ROUTER_MAX_CHILDREN];  // 子节点
    size_t child_count;
    int is_param;                         // 是否参数节点
    char param_name[MAX_PARAM_NAME_LEN];  // 参数名
} uvhttp_route_node_t;

// 数组路由结构
typedef struct {
    char path[MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
} array_route_t;

// 路由器结构
struct uvhttp_router {
    /* 热路径字段（频繁访问）- 优化内存局部性 */
    int use_trie;       /* 4 字节 - 是否使用Trie */
    size_t route_count; /* 8 字节 - 总路由数量 */

    /* Trie路由相关（8字节对齐） */
    uvhttp_route_node_t* root;      /* 8 字节 */
    uvhttp_route_node_t* node_pool; /* 8 字节 */
    size_t node_pool_size;          /* 8 字节 */
    size_t node_pool_used;          /* 8 字节 */

    /* 数组路由相关（8字节对齐） */
    array_route_t* array_routes; /* 8 字节 */
    size_t array_route_count;    /* 8 字节 */
    size_t array_capacity;       /* 8 字节 */

    /* 静态文件路由支持（8字节对齐） */
    char* static_prefix;                     /* 8 字节 */
    void* static_context;                    /* 8 字节 */
    uvhttp_request_handler_t static_handler; /* 8 字节 */

    /* 回退路由支持（8字节对齐） */
    void* fallback_context;                    /* 8 字节 */
    uvhttp_request_handler_t fallback_handler; /* 8 字节 */
};

typedef struct uvhttp_router uvhttp_router_t;

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, root) % 8 == 0,
                     "root pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, node_pool) % 8 == 0,
                     "node_pool pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, array_routes) % 8 == 0,
                     "array_routes pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, static_prefix) % 8 == 0,
                     "static_prefix pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, static_context) % 8 == 0,
                     "static_context pointer not 8-byte aligned");

/* 验证size_t对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, route_count) % 8 == 0,
                     "route_count not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_router_t, node_pool_size) % 8 == 0,
                     "node_pool_size not 8-byte aligned");

// 路由API函数
/**
 * @brief 创建新的路由器
 * @param router 输出参数，用于接收路由器指针
 * @return UVHTTP_OK 成功，其他值表示失败
 * @note 成功时，*router 被设置为有效的路由器对象，必须使用 uvhttp_router_free
 * 释放
 * @note 失败时，*router 被设置为 NULL
 */
uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router);
void uvhttp_router_free(uvhttp_router_t* router);

// 路由添加（支持HTTP方法）
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router,
                                              const char* path,
                                              uvhttp_method_t method,
                                              uvhttp_request_handler_t handler);

// 路由查找
uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method);

// 路由匹配（获取参数）
uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                   const char* path, const char* method,
                                   uvhttp_route_match_t* match);

// 参数解析
uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count);

// 方法字符串转换
uvhttp_method_t uvhttp_method_from_string(const char* method);
const char* uvhttp_method_to_string(uvhttp_method_t method);

// 静态文件路由支持
uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                              const char* prefix_path,
                                              void* static_context);

// 回退路由支持
uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                void* static_context);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ROUTER_H */