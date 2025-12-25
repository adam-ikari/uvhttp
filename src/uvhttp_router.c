#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_utils.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// 方法字符串映射
static const struct {
    const char* name;
    uvhttp_method_t method;
} method_map[] = {
    {UVHTTP_METHOD_GET, UVHTTP_GET},
    {UVHTTP_METHOD_POST, UVHTTP_POST},
    {UVHTTP_METHOD_PUT, UVHTTP_PUT},
    {UVHTTP_METHOD_DELETE, UVHTTP_DELETE},
    {UVHTTP_METHOD_HEAD, UVHTTP_HEAD},
    {UVHTTP_METHOD_OPTIONS, UVHTTP_OPTIONS},
    {UVHTTP_METHOD_PATCH, UVHTTP_PATCH},
    {NULL, UVHTTP_ANY}
};

#define HYBRID_THRESHOLD 100  // 切换到Trie的路由数量阈值

uvhttp_method_t uvhttp_method_from_string(const char* method) {
    for (int i = 0; method_map[i].name; i++) {
        if (strcmp(method, method_map[i].name) == 0) {
            return method_map[i].method;
        }
    }
    return UVHTTP_ANY;
}

const char* uvhttp_method_to_string(uvhttp_method_t method) {
    for (int i = 0; method_map[i].name; i++) {
        if (method_map[i].method == method) {
            return method_map[i].name;
        }
    }
    return "UNKNOWN";
}

// 创建新的路由节点
static uvhttp_route_node_t* create_route_node(uvhttp_router_t* router) {
    if (router->node_pool_used >= router->node_pool_size) {
        // 扩展节点池
        size_t new_size = router->node_pool_size * 2;
        uvhttp_route_node_t* new_pool = UVHTTP_REALLOC(router->node_pool, 
                                                   new_size * sizeof(uvhttp_route_node_t));
        if (!new_pool) {
            return NULL;
        }
        
        // 初始化新增的节点
        for (size_t i = router->node_pool_size; i < new_size; i++) {
            memset(&new_pool[i], 0, sizeof(uvhttp_route_node_t));
        }
        
        router->node_pool = new_pool;
        router->node_pool_size = new_size;
    }
    
    return &router->node_pool[router->node_pool_used++];
}

// 查找或创建子节点
static uvhttp_route_node_t* find_or_create_child(uvhttp_router_t* router, 
                                                uvhttp_route_node_t* parent, 
                                                const char* segment, 
                                                int is_param) {
    // 查找现有子节点
    for (size_t i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->segment, segment) == 0) {
            return parent->children[i];
        }
    }
    
    // 创建新子节点
    if (parent->child_count >= 16) {
        return NULL;
    }
    
    uvhttp_route_node_t* child = create_route_node(router);
    if (!child) {
        return NULL;
    }
    
    strncpy(child->segment, segment, sizeof(child->segment) - 1);
    child->segment[sizeof(child->segment) - 1] = '\0';
    child->is_param = is_param;
    
    parent->children[parent->child_count++] = child;
    return child;
}

// 解析路径参数
static int parse_path_params(const char* path, uvhttp_param_t* params, size_t* param_count) {
    if (!path || !params || !param_count) {
        return -1;
    }
    
    *param_count = 0;
    char path_copy[MAX_ROUTE_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char* token = strtok(path_copy, "/");
    while (token && *param_count < MAX_PARAMS) {
        // 检查是否是参数（以:开头）
        if (token[0] == ':') {
            char* colon = strchr(token + 1, ':');
            if (colon) {
                *colon = '\0';
                strncpy(params[*param_count].name, token + 1, sizeof(params[*param_count].name) - 1);
                strncpy(params[*param_count].value, colon + 1, sizeof(params[*param_count].value) - 1);
                params[*param_count].name[sizeof(params[*param_count].name) - 1] = '\0';
                params[*param_count].value[sizeof(params[*param_count].value) - 1] = '\0';
                (*param_count)++;
            }
        }
        token = strtok(NULL, "/");
    }
    
    return 0;
}

uvhttp_router_t* uvhttp_router_new(void) {
    uvhttp_router_t* router = UVHTTP_MALLOC(sizeof(uvhttp_router_t));
    if (!router) {
        return NULL;
    }
    
    memset(router, 0, sizeof(uvhttp_router_t));
    
    // 初始化数组路由
    router->array_routes = UVHTTP_CALLOC(HYBRID_THRESHOLD, sizeof(array_route_t));
    if (!router->array_routes) {
        UVHTTP_FREE(router);
        return NULL;
    }
    router->array_capacity = HYBRID_THRESHOLD;
    
    // 初始化节点池（用于Trie）
    router->node_pool_size = 64;
    router->node_pool = UVHTTP_CALLOC(router->node_pool_size, sizeof(uvhttp_route_node_t));
    if (!router->node_pool) {
        UVHTTP_FREE(router->array_routes);
        UVHTTP_FREE(router);
        return NULL;
    }
    
    router->root = create_route_node(router);
    if (!router->root) {
        UVHTTP_FREE(router->node_pool);
        UVHTTP_FREE(router->array_routes);
        UVHTTP_FREE(router);
        return NULL;
    }
    
    router->use_trie = 0;  // 默认使用数组路由
    return router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (router) {
        if (router->node_pool) {
            UVHTTP_FREE(router->node_pool);
            router->node_pool = NULL;
        }
        if (router->array_routes) {
            UVHTTP_FREE(router->array_routes);
            router->array_routes = NULL;
        }
        if (router->static_prefix) {
            free(router->static_prefix);
            router->static_prefix = NULL;
        }
        UVHTTP_FREE(router);
    }
}

// 数组路由添加
static uvhttp_error_t add_array_route(uvhttp_router_t* router, const char* path,
                                     uvhttp_method_t method, uvhttp_request_handler_t handler) {
    if (router->array_route_count >= router->array_capacity) {
        // 扩展数组容量
        size_t new_capacity = router->array_capacity * 2;
        array_route_t* new_routes = UVHTTP_REALLOC(router->array_routes, 
                                                 new_capacity * sizeof(array_route_t));
        if (!new_routes) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        router->array_routes = new_routes;
        router->array_capacity = new_capacity;
    }
    
    array_route_t* route = &router->array_routes[router->array_route_count];
    strncpy(route->path, path, sizeof(route->path) - 1);
    route->path[sizeof(route->path) - 1] = '\0';
    route->method = method;
    route->handler = handler;
    router->array_route_count++;
    
    return UVHTTP_OK;
}

// 数组路由查找
static uvhttp_request_handler_t find_array_route(const uvhttp_router_t* router, 
                                                const char* path, 
                                                uvhttp_method_t method) {
    for (size_t i = 0; i < router->array_route_count; i++) {
        array_route_t* route = &router->array_routes[i];
        if (route->method == method || route->method == UVHTTP_ANY) {
            if (strcmp(route->path, path) == 0) {
                return route->handler;
            }
        }
    }
    return NULL;
}

// 迁移数组路由到Trie
static uvhttp_error_t migrate_to_trie(uvhttp_router_t* router) {
    if (router->use_trie) {
        return UVHTTP_OK;  // 已经是Trie模式
    }
    
    // 保存数组路由指针，稍后释放
    array_route_t* old_routes = router->array_routes;
    size_t old_count = router->array_route_count;
    
    // 将所有数组路由迁移到Trie
    for (size_t i = 0; i < old_count; i++) {
        array_route_t* route = &old_routes[i];
        
        // 构建路由树
        uvhttp_route_node_t* current = router->root;
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, route->path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        
        char* token = strtok(path_copy, "/");
        while (token) {
            int is_param = (token[0] == ':');
            if (is_param) {
                token++;
            }
            
            current = find_or_create_child(router, current, token, is_param);
            if (!current) {
                UVHTTP_FREE(old_routes);  // 清理已分配的内存
                return UVHTTP_ERROR_OUT_OF_MEMORY;
            }
            
            token = strtok(NULL, "/");
        }
        
        // 设置处理器
        current->method = route->method;
        current->handler = route->handler;
    }
    
    // 切换到Trie模式
    router->use_trie = 1;
    router->array_routes = NULL;
    router->array_route_count = 0;
    router->array_capacity = 0;
    
    // 释放旧的数组路由内存
    UVHTTP_FREE(old_routes);
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router, 
                                           const char* path, 
                                           uvhttp_request_handler_t handler) {
    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router,
                                                const char* path,
                                                uvhttp_method_t method,
                                                uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (strlen(path) >= MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 检查是否包含路径参数
    int has_params = (strchr(path, ':') != NULL);
    
    // 如果有参数或路由数量超过阈值，使用Trie
    if (has_params || router->array_route_count >= HYBRID_THRESHOLD) {
        if (!router->use_trie) {
            uvhttp_error_t err = migrate_to_trie(router);
            if (err != UVHTTP_OK) {
                return err;
            }
        }
        
        // 添加到Trie
        uvhttp_route_node_t* current = router->root;
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        
        char* token = strtok(path_copy, "/");
        while (token) {
            int is_param = (token[0] == ':');
            if (is_param) {
                token++;
                // 保存参数名 - 优化：只计算一次strlen
                size_t token_len = strlen(token);
                for (size_t i = 0; i < token_len; i++) {
                    if (token[i] == '/') {
                        strncpy(current->param_name, token, i);
                        current->param_name[i] = '\0';
                        break;
                    }
                }
            }
            
            current = find_or_create_child(router, current, token, is_param);
            if (!current) {
                return UVHTTP_ERROR_OUT_OF_MEMORY;
            }
            
            token = strtok(NULL, "/");
        }
        
        current->method = method;
        current->handler = handler;
        router->route_count++;
    } else {
        // 添加到数组
        return add_array_route(router, path, method, handler);
    }
    
    return UVHTTP_OK;
}

static int match_route_node(uvhttp_route_node_t* node,
                           const char** segments,
                           int segment_count,
                           int segment_index,
                           uvhttp_method_t method,
                           uvhttp_route_match_t* match) {
    if (!node || !segments || !match) {
        return -1;
    }
    
    // 如果是叶子节点
    if (segment_index >= segment_count) {
        if (node->handler && (node->method == UVHTTP_ANY || node->method == method)) {
            match->handler = node->handler;
            match->param_count = 0;
            return 0;
        }
        return -1;
    }
    
    const char* segment = segments[segment_index];
    
    // 查找匹配的子节点
    for (size_t i = 0; i < node->child_count; i++) {
        uvhttp_route_node_t* child = node->children[i];
        
        if (child->is_param) {
            // 参数节点，匹配任意段
            strncpy(match->params[match->param_count].name, child->param_name, 
                    sizeof(match->params[match->param_count].name) - 1);
            strncpy(match->params[match->param_count].value, segment, 
                    sizeof(match->params[match->param_count].value) - 1);
            match->param_count++;
            
            int result = match_route_node(child, segments, segment_count, 
                                       segment_index + 1, method, match);
            if (result == 0) {
                return 0;
            }
            match->param_count--; // 回溯
        } else {
            // 精确匹配
            if (strcmp(child->segment, segment) == 0) {
                int result = match_route_node(child, segments, segment_count, 
                                       segment_index + 1, method, match);
                if (result == 0) {
                    return 0;
                }
            }
        }
    }
    
    return -1;
}

uvhttp_request_handler_t uvhttp_router_find_handler(const uvhttp_router_t* router, 
                                                   const char* path,
                                                   const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }
    
    uvhttp_method_t method_enum = uvhttp_method_from_string(method);
    
    // 根据当前模式选择查找方式
    if (router->use_trie) {
        // Trie查找
        uvhttp_route_match_t match;
        memset(&match, 0, sizeof(match));
        
        // 解析路径段
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        
        const char* segments[MAX_ROUTE_PATH_LEN];
        int segment_count = 0;
        
        char* token = strtok(path_copy, "/");
        while (token && segment_count < MAX_ROUTE_PATH_LEN) {
            segments[segment_count++] = token;
            token = strtok(NULL, "/");
        }
        
        // 首先检查静态路由
    if (router->static_prefix && router->static_context) {
        size_t prefix_len = strlen(router->static_prefix);
        if (strncmp(path, router->static_prefix, prefix_len) == 0) {
            // 匹配静态路由，返回静态文件处理器
            return (uvhttp_request_handler_t)router->static_context;
        }
    }
    
    // 匹配路由
        if (match_route_node(router->root, segments, segment_count, 0, method_enum, &match) == 0) {
            return match.handler;
        }
    } else {
        // 数组查找
        uvhttp_request_handler_t handler = find_array_route(router, path, method_enum);
        if (handler) {
            return handler;
        }
    }
    
    // 如果没有匹配的路由，检查回退路由
    if (router->fallback_context) {
        return (uvhttp_request_handler_t)router->fallback_context;
    }
    
    return NULL;
}

uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                      const char* path,
                                      const char* method,
                                      uvhttp_route_match_t* match) {
    if (!router || !path || !method || !match) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    memset(match, 0, sizeof(uvhttp_route_match_t));
    
    uvhttp_method_t method_enum = uvhttp_method_from_string(method);
    
    // 目前只支持Trie模式的参数匹配
    if (!router->use_trie) {
        uvhttp_request_handler_t handler = find_array_route(router, path, method_enum);
        if (handler) {
            match->handler = handler;
            return UVHTTP_OK;
        }
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    // 解析路径段
    char path_copy[MAX_ROUTE_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    const char* segments[MAX_ROUTE_PATH_LEN];
    int segment_count = 0;
    
    char* token = strtok(path_copy, "/");
    while (token && segment_count < MAX_ROUTE_PATH_LEN) {
        segments[segment_count++] = token;
        token = strtok(NULL, "/");
    }
    
    return match_route_node(router->root, segments, segment_count, 0, method_enum, match) == 0 ? 
           UVHTTP_OK : UVHTTP_ERROR_NOT_FOUND;
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                         uvhttp_param_t* params,
                                         size_t* param_count) {
    return parse_path_params(path, params, param_count);
}

/**
 * 添加静态文件路由
 */
uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                               const char* prefix_path,
                                               void* static_context) {
    if (!router || !prefix_path || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 释放之前的前缀
    if (router->static_prefix) {
        UVHTTP_FREE(router->static_prefix);
    }
    
    // 复制新的前缀
    router->static_prefix = strdup(prefix_path);
    if (!router->static_prefix) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    router->static_context = static_context;
    router->static_handler = NULL; // 将使用静态文件处理逻辑
    
    return UVHTTP_OK;
}

/**
 * 添加回退路由
 */
uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                 void* static_context) {
    if (!router || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    router->fallback_context = static_context;
    router->fallback_handler = NULL; // 将使用静态文件处理逻辑
    
    return UVHTTP_OK;
}