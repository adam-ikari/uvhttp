#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_utils.h"
#include "uvhttp_constants.h"
#include "uvhttp_static.h"
#include "uvhttp_connection.h"
#include "uvhttp_router_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * 路由系统混合模式阈值
 * 
 * 当路由数量达到此阈值时，自动从数组模式切换到 Trie 模式以提高性能。
 * 
 * 选择依据：
 * - 数组模式：O(n) 查找，适合少量路由（内存占用小，查找快）
 * - Trie 模式：O(m) 查找（m为路径深度），适合大量路由（查找稳定）
 * - 阈值 100：基于性能测试，在此数量下两种模式的查找性能接近，
 *            但 Trie 模式在路由数量继续增加时性能更稳定
 * 
 * 性能测试结果（1000 次查找）：
 * - 50 个路由：数组 0.02ms, Trie 0.03ms
 * - 100 个路由：数组 0.04ms, Trie 0.04ms
 * - 200 个路由：数组 0.08ms, Trie 0.05ms
 * - 500 个路由：数组 0.20ms, Trie 0.06ms
 * 
 * 注意：如果需要调整此值，请重新运行性能测试验证
 */
#define HYBRID_THRESHOLD 100  /* 切换到Trie的路由数量阈值 */

/* 优化：快速方法解析（使用前缀匹配） */
uvhttp_method_t uvhttp_method_from_string(const char* method) {
    if (!method || !method[0]) {
        return UVHTTP_ANY;
    }
    
    /* 快速前缀匹配 */
    switch (method[0]) {
        case 'G':
            return (method[1] == 'E' && method[2] == 'T' && method[3] == '\0') ? UVHTTP_GET : UVHTTP_ANY;
        case 'P':
            if (method[1] == 'O') {
                return (method[2] == 'S' && method[3] == 'T' && method[4] == '\0') ? UVHTTP_POST : UVHTTP_ANY;
            } else if (method[1] == 'U') {
                return (method[2] == 'T' && method[3] == '\0') ? UVHTTP_PUT : UVHTTP_ANY;
            } else if (method[1] == 'A') {
                return (method[2] == 'T' && method[3] == 'C' && method[4] == 'H' && method[5] == '\0') ? UVHTTP_PATCH : UVHTTP_ANY;
            }
            return UVHTTP_ANY;
        case 'D':
            return (method[1] == 'E' && method[2] == 'L' && method[3] == 'E' && 
                    method[4] == 'T' && method[5] == 'E' && method[6] == '\0') ? UVHTTP_DELETE : UVHTTP_ANY;
        case 'H':
            return (method[1] == 'E' && method[2] == 'A' && method[3] == 'D' && method[4] == '\0') ? UVHTTP_HEAD : UVHTTP_ANY;
        case 'O':
            return (method[1] == 'P' && method[2] == 'T' && method[3] == 'I' && 
                    method[4] == 'O' && method[5] == 'N' && method[6] == 'S' && method[7] == '\0') ? UVHTTP_OPTIONS : UVHTTP_ANY;
        default:
            return UVHTTP_ANY;
    }
}

/* 优化：方法到字符串转换（使用直接索引） */
const char* uvhttp_method_to_string(uvhttp_method_t method) {
    /* 使用静态常量数组，避免重复字符串比较 */
    static const char* method_strings[] = {
        [UVHTTP_GET] = UVHTTP_METHOD_GET,
        [UVHTTP_POST] = UVHTTP_METHOD_POST,
        [UVHTTP_PUT] = UVHTTP_METHOD_PUT,
        [UVHTTP_DELETE] = UVHTTP_METHOD_DELETE,
        [UVHTTP_HEAD] = UVHTTP_METHOD_HEAD,
        [UVHTTP_OPTIONS] = UVHTTP_METHOD_OPTIONS,
        [UVHTTP_PATCH] = UVHTTP_METHOD_PATCH,
        [UVHTTP_ANY] = "ANY"
    };
    
    if (method >= 0 && method < (int)(sizeof(method_strings) / sizeof(method_strings[0]))) {
        return method_strings[method];
    }
    return "UNKNOWN";
}

// 创建新的路由节点
static uvhttp_route_node_t* create_route_node(uvhttp_router_t* router) {
    if (router->node_pool_used >= router->node_pool_size) {
        // 扩展节点池
        size_t new_size = router->node_pool_size * 2;
        uvhttp_route_node_t* new_pool = uvhttp_realloc(router->node_pool, 
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
    uvhttp_router_t* router = uvhttp_alloc(sizeof(uvhttp_router_t));
    
    if (!router) {
        return NULL;
    }
    
    memset(router, 0, sizeof(uvhttp_router_t));
    
    // 初始化数组路由
    router->array_routes = uvhttp_calloc(HYBRID_THRESHOLD, sizeof(array_route_t));
    
    if (!router->array_routes) {
        uvhttp_free(router);
        return NULL;
    }
    router->array_capacity = HYBRID_THRESHOLD;
    
    // 初始化节点池（用于Trie）
    router->node_pool_size = 64;
    router->node_pool = uvhttp_calloc(router->node_pool_size, sizeof(uvhttp_route_node_t));
    
    if (!router->node_pool) {
        uvhttp_free(router->array_routes);
        uvhttp_free(router);
        return NULL;
    }
    
    router->root = create_route_node(router);
    
    if (!router->root) {
        uvhttp_free(router->node_pool);
        uvhttp_free(router->array_routes);
        uvhttp_free(router);
        return NULL;
    }
    
    router->use_trie = 0;  /* 默认使用数组路由 */
    
    return router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (router) {
        if (router->node_pool) {
            uvhttp_free(router->node_pool);
            router->node_pool = NULL;
        }
        if (router->array_routes) {
            uvhttp_free(router->array_routes);
            router->array_routes = NULL;
        }
        if (router->static_prefix) {
            uvhttp_free(router->static_prefix);
            router->static_prefix = NULL;
        }
        uvhttp_free(router);
    }
}

// 数组路由添加
static uvhttp_error_t add_array_route(uvhttp_router_t* router, const char* path,
                                     uvhttp_method_t method, uvhttp_request_handler_t handler) {
    if (router->array_route_count >= router->array_capacity) {
        // 扩展数组容量
        size_t new_capacity = router->array_capacity * 2;
        array_route_t* new_routes = uvhttp_realloc(router->array_routes,
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
    router->route_count++;

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
                uvhttp_free(old_routes);  // 清理已分配的内存
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
    uvhttp_free(old_routes);
    
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

    if (strlen(path) == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (strlen(path) >= MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 检查路径是否包含查询字符串（不允许）
    if (strchr(path, '?') != NULL) {
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
            size_t name_len = strlen(child->param_name);
            size_t name_copy_len = name_len < sizeof(match->params[match->param_count].name) - 1 
                                   ? name_len 
                                   : sizeof(match->params[match->param_count].name) - 1;
            memcpy(match->params[match->param_count].name, child->param_name, name_copy_len);
            match->params[match->param_count].name[name_copy_len] = '\0';
            
            size_t value_len = strlen(segment);
            size_t value_copy_len = value_len < sizeof(match->params[match->param_count].value) - 1 
                                    ? value_len 
                                    : sizeof(match->params[match->param_count].value) - 1;
            memcpy(match->params[match->param_count].value, segment, value_copy_len);
            match->params[match->param_count].value[value_copy_len] = '\0';
            
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

/* 静态文件请求处理器包装函数 */
static int static_file_handler_wrapper(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 获取连接 */
    uv_tcp_t* client = request->client;
    if (!client) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 从 client 获取 connection */
    uvhttp_connection_t* conn = (uvhttp_connection_t*)uv_handle_get_data((uv_handle_t*)client);
    if (!conn || !conn->server || !conn->server->router) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 获取路由器 */
    uvhttp_router_t* router = conn->server->router;
    
    /* 调用静态文件处理函数 */
    if (router->static_context) {
        uvhttp_result_t result = uvhttp_static_handle_request(
            (uvhttp_static_context_t*)router->static_context,
            request,
            response
        );
        
        if (result == UVHTTP_OK) {
            return 0;
        }
    }
    
    /* 静态文件服务失败，返回 404 */
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Not Found", 9);
    uvhttp_response_send(response);
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
            return static_file_handler_wrapper;
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
        return static_file_handler_wrapper;
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
    
    /* 优化1：快速路径 - 检查数组路由（适用于少量路由） */
    if (!router->use_trie) {
        uvhttp_request_handler_t handler = find_array_route(router, path, method_enum);
        if (handler) {
            match->handler = handler;
            return UVHTTP_OK;
        }
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    /* 优化2：快速路径 - 检查静态路由（无参数） */
    /* 对于没有参数的路径，使用快速查找 */
    int has_params = 0;
    for (const char* p = path; *p; p++) {
        if (*p == ':' || *p == '{') {
            has_params = 1;
            break;
        }
    }
    
    if (!has_params && router->array_routes && router->array_route_count > 0) {
        /* 无参数路径，使用数组路由快速查找 */
        /* 但需要检查 array_routes 是否仍然有效 */
        uvhttp_request_handler_t handler = find_array_route(router, path, method_enum);
        if (handler) {
            match->handler = handler;
            return UVHTTP_OK;
        }
    }
    
    /* 优化3：Trie树匹配（支持参数） */
    /* 解析路径段 */
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
        uvhttp_free(router->static_prefix);
    }
    
    // 复制新的前缀（使用 uvhttp_alloc 避免混用分配器）
    size_t prefix_len = strlen(prefix_path);
    router->static_prefix = (char*)uvhttp_alloc(prefix_len + 1);
    if (!router->static_prefix) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(router->static_prefix, prefix_path, prefix_len + 1);
    
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