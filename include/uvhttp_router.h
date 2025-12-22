#ifndef UVHTTP_ROUTER_H
#define UVHTTP_ROUTER_H

#include <stddef.h>
#include "uvhttp_error.h"
#include "uvhttp_common.h"

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 128
#define MAX_ROUTE_PATH_LEN 256
#define MAX_PARAMS 16
#define MAX_PARAM_NAME_LEN 64
#define MAX_PARAM_VALUE_LEN 256

// HTTP方法枚举
typedef enum {
    UVHTTP_METHOD_ANY = 0,
    UVHTTP_METHOD_GET,
    UVHTTP_METHOD_POST,
    UVHTTP_METHOD_PUT,
    UVHTTP_METHOD_DELETE,
    UVHTTP_METHOD_HEAD,
    UVHTTP_METHOD_OPTIONS,
    UVHTTP_METHOD_PATCH
} uvhttp_method_t;

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
    struct uvhttp_route_node* children[16];  // 子节点
    size_t child_count;
    int is_param;  // 是否参数节点
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
    // Trie路由相关
    uvhttp_route_node_t* root;
    uvhttp_route_node_t* node_pool;
    size_t node_pool_size;
    size_t node_pool_used;
    
    // 数组路由相关
    array_route_t* array_routes;
    size_t array_route_count;
    size_t array_capacity;
    
    // 混合模式标志
    int use_trie;
    
    // 总路由数量
    size_t route_count;
};

typedef struct uvhttp_router uvhttp_router_t;

// 路由API函数
uvhttp_router_t* uvhttp_router_new(void);
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
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, 
                                                   const char* path,
                                                   const char* method);

// 路由匹配（获取参数）
uvhttp_error_t uvhttp_router_match(uvhttp_router_t* router,
                                      const char* path,
                                      const char* method,
                                      uvhttp_route_match_t* match);

// 参数解析
uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                         uvhttp_param_t* params,
                                         size_t* param_count);

// 方法字符串转换
uvhttp_method_t uvhttp_method_from_string(const char* method);
const char* uvhttp_method_to_string(uvhttp_method_t method);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ROUTER_H */