#if UVHTTP_FEATURE_ROUTER_CACHE

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_hash.h"
#include "uvhttp_router.h"
#include "uvhttp_utils.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 哈希桶结构 */
typedef struct route_hash_entry {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    size_t access_count;
    struct route_hash_entry* next;
} route_hash_entry_t;

/* 缓存优化的路由器结构 - 采用分层缓存策略提高性能
 * 
 * 线程安全说明：
 * 本实现专为单线程事件循环架构设计（基于 libuv）：
 * - 路由器必须在单个事件循环线程中使用
 * - 不支持多线程并发访问
 * - 如果需要在多线程环境中使用，请为每个线程创建独立的服务器实例和路由器
 * 
 * 性能优化：
 * - 采用分层缓存策略（热路径 + 哈希表）
 * - 热路径缓存利用 CPU 缓存局部性
 * - 无锁设计，避免互斥锁开销
 */
typedef struct {
    /* 热路径缓存：存储最常用的16个路由，利用CPU缓存局部性 */
    struct {
        char path[UVHTTP_MAX_ROUTE_PATH_LEN];
        uvhttp_method_t method;
        uvhttp_request_handler_t handler;
        size_t access_count;
    } hot_routes[UVHTTP_ROUTER_HOT_ROUTES_COUNT];
    size_t hot_count;
    size_t hot_index;  /* 用于循环替换的索引 */

    /* 哈希表：用于快速查找所有路由（包括冷路径） */
    route_hash_entry_t* hash_table[UVHTTP_ROUTER_HASH_SIZE];

    /* 路由统计 */
    size_t total_routes;
} cache_optimized_router_t;

/* 使用统一的hash函数（内联函数） */
static inline uint32_t route_hash(const char* str) {
    if (!str)
        return 0;

    size_t len = strlen(str);
    if (len > UVHTTP_MAX_ROUTE_PATH_LEN - 1) {
        len = UVHTTP_MAX_ROUTE_PATH_LEN - 1;
    }

    return (uint32_t)XXH64(str, len, UVHTTP_HASH_DEFAULT_SEED);
}

/* 快速方法解析（内联函数） */
static inline uvhttp_method_t fast_method_parse(const char* method) {
    if (!method)
        return UVHTTP_ANY;

    /* 使用前缀快速判断 */
    switch (method[0]) {
    case 'G':
        return (method[1] == 'E' && method[2] == 'T' && method[3] == '\0')
                   ? UVHTTP_GET
                   : UVHTTP_ANY;
    case 'P':
        if (method[1] == 'O') {
            return (method[2] == 'S' && method[3] == 'T' && method[4] == '\0')
                       ? UVHTTP_POST
                       : UVHTTP_ANY;
        } else if (method[1] == 'U') {
            return (method[2] == 'T' && method[3] == '\0')
                       ? UVHTTP_PUT
                       : UVHTTP_ANY;
        }
        break;
    case 'D':
        return (method[1] == 'E' && method[2] == 'L' && method[3] == 'E' &&
                method[4] == 'T' && method[5] == 'E' && method[6] == '\0')
                   ? UVHTTP_DELETE
                   : UVHTTP_ANY;
    case 'H':
        return (method[1] == 'E' && method[2] == 'A' && method[3] == 'D' &&
                method[4] == '\0')
                   ? UVHTTP_HEAD
                   : UVHTTP_ANY;
    case 'O':
        return (method[1] == 'P' && method[2] == 'T' && method[3] == 'I' &&
                method[4] == 'O' && method[5] == 'N' && method[6] == 'S' &&
                method[7] == '\0')
                   ? UVHTTP_OPTIONS
                   : UVHTTP_ANY;
    case 'P':
        return (method[1] == 'A' && method[2] == 'T' && method[3] == 'C' &&
                method[4] == 'H' && method[5] == '\0')
                   ? UVHTTP_PATCH
                   : UVHTTP_ANY;
    case 'T':
        return (method[1] == 'R' && method[2] == 'A' && method[3] == 'C' &&
                method[4] == 'E' && method[5] == '\0')
                   ? UVHTTP_TRACE
                   : UVHTTP_ANY;
    case 'C':
        return (method[1] == 'O' && method[2] == 'N' && method[3] == 'N' &&
                method[4] == 'E' && method[5] == 'C' && method[6] == 'T' &&
                method[7] == '\0')
                   ? UVHTTP_CONNECT
                   : UVHTTP_ANY;
    }
    return UVHTTP_ANY;
}

/* 添加到哈希表 */
static uvhttp_error_t add_to_hash_table(cache_optimized_router_t* cr,
                                         const char* path,
                                         uvhttp_method_t method,
                                         uvhttp_request_handler_t handler) {
    if (!cr || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查路径长度 */
    if (strlen(path) >= UVHTTP_MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_HEADER_TOO_LARGE;
    }

    uint32_t hash = route_hash(path);
    uint32_t index = hash % UVHTTP_ROUTER_HASH_SIZE;

    /* 检查是否已存在相同的路由 */
    route_hash_entry_t* entry = cr->hash_table[index];
    while (entry) {
        if (strcmp(entry->path, path) == 0 && entry->method == method) {
            return UVHTTP_ERROR_ALREADY_EXISTS;
        }
        entry = entry->next;
    }

    /* 创建新条目 */
    route_hash_entry_t* new_entry = uvhttp_alloc(sizeof(route_hash_entry_t));
    if (!new_entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(new_entry, 0, sizeof(route_hash_entry_t));
    uvhttp_safe_strncpy(new_entry->path, UVHTTP_MAX_ROUTE_PATH_LEN, path);
    new_entry->method = method;
    new_entry->handler = handler;
    new_entry->access_count = 0;
    new_entry->next = cr->hash_table[index];
    cr->hash_table[index] = new_entry;

    cr->total_routes++;
    return UVHTTP_OK;
}

/* 添加到热路径 */
static uvhttp_error_t add_to_hot_routes(cache_optimized_router_t* cr,
                                        const char* path,
                                        uvhttp_method_t method,
                                        uvhttp_request_handler_t handler) {
    if (!cr || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查路径长度 */
    if (strlen(path) >= UVHTTP_MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_HEADER_TOO_LARGE;
    }

    /* 检查是否已存在 */
    for (size_t i = 0; i < cr->hot_count; i++) {
        if (strcmp(cr->hot_routes[i].path, path) == 0 &&
            cr->hot_routes[i].method == method) {
            return UVHTTP_ERROR_ALREADY_EXISTS;
        }
    }

    /* 如果热路径未满，直接添加 */
    if (cr->hot_count < UVHTTP_ROUTER_HOT_ROUTES_COUNT) {
        uvhttp_safe_strncpy(cr->hot_routes[cr->hot_count].path,
                           UVHTTP_MAX_ROUTE_PATH_LEN, path);
        cr->hot_routes[cr->hot_count].method = method;
        cr->hot_routes[cr->hot_count].handler = handler;
        cr->hot_routes[cr->hot_count].access_count = 0;
        cr->hot_count++;
        return UVHTTP_OK;
    }

    /* 热路径已满，替换访问次数最少的（循环替换策略） */
    size_t min_index = cr->hot_index;
    size_t min_count = cr->hot_routes[min_index].access_count;

    for (size_t i = 1; i < UVHTTP_ROUTER_HOT_ROUTES_COUNT; i++) {
        if (cr->hot_routes[i].access_count < min_count) {
            min_count = cr->hot_routes[i].access_count;
            min_index = i;
        }
    }

    /* 替换 */
    uvhttp_safe_strncpy(cr->hot_routes[min_index].path,
                       UVHTTP_MAX_ROUTE_PATH_LEN, path);
    cr->hot_routes[min_index].method = method;
    cr->hot_routes[min_index].handler = handler;
    cr->hot_routes[min_index].access_count = 0;

    /* 更新循环索引 */
    cr->hot_index = (min_index + 1) % UVHTTP_ROUTER_HOT_ROUTES_COUNT;

    return UVHTTP_OK;
}

/* 在哈希表中查找 */
static uvhttp_request_handler_t find_in_hash_table(
    cache_optimized_router_t* cr, const char* path, uvhttp_method_t method) {
    if (!cr || !path) {
        return NULL;
    }

    uint32_t hash = route_hash(path);
    uint32_t index = hash % UVHTTP_ROUTER_HASH_SIZE;

    route_hash_entry_t* entry = cr->hash_table[index];
    while (entry) {
        if (strcmp(entry->path, path) == 0 && entry->method == method) {
            entry->access_count++;
            return entry->handler;
        }
        entry = entry->next;
    }

    return NULL;
}

/* 在热路径中查找 */
static uvhttp_request_handler_t find_in_hot_routes(
    cache_optimized_router_t* cr, const char* path, uvhttp_method_t method) {
    if (!cr || !path) {
        return NULL;
    }

    for (size_t i = 0; i < cr->hot_count; i++) {
        if (strcmp(cr->hot_routes[i].path, path) == 0 &&
            cr->hot_routes[i].method == method) {
            cr->hot_routes[i].access_count++;
            return cr->hot_routes[i].handler;
        }
    }

    return NULL;
}

/* ========== 公开 API 函数 ========== */

uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router) {
    if (!router) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr =
        uvhttp_calloc(1, sizeof(cache_optimized_router_t));
    if (!cr) {
        *router = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(cr, 0, sizeof(cache_optimized_router_t));

    *router = (uvhttp_router_t*)cr;
    return UVHTTP_OK;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (!router) {
        return;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* 释放哈希表 */
    for (size_t i = 0; i < UVHTTP_ROUTER_HASH_SIZE; i++) {
        route_hash_entry_t* entry = cr->hash_table[i];
        while (entry) {
            route_hash_entry_t* next = entry->next;
            uvhttp_free(entry);
            entry = next;
        }
        cr->hash_table[i] = NULL;
    }

    uvhttp_free(cr);
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router,
                                              const char* path,
                                              uvhttp_method_t method,
                                              uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* 添加到哈希表 */
    uvhttp_error_t err = add_to_hash_table(cr, path, method, handler);
    if (err == UVHTTP_OK) {
        /* 添加到热路径 */
        add_to_hot_routes(cr, path, method, handler);
    }

    return err;
}

uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    /* 先在热路径中查找 */
    uvhttp_request_handler_t handler = find_in_hot_routes(cr, path, method_enum);

    /* 如果热路径中没找到，在哈希表中查找 */
    if (!handler) {
        handler = find_in_hash_table(cr, path, method_enum);
    }

    return handler;
}

uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                   const char* path, const char* method,
                                   uvhttp_route_match_t* match) {
    if (!router || !path || !method || !match) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    /* 先在热路径中查找 */
    uvhttp_request_handler_t handler = find_in_hot_routes(cr, path, method_enum);

    /* 如果热路径中没找到，在哈希表中查找 */
    if (!handler) {
        handler = find_in_hash_table(cr, path, method_enum);
    }

    if (!handler) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    match->handler = handler;
    match->param_count = 0;

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count) {
    if (!path || !params || !param_count) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *param_count = 0;
    return UVHTTP_OK;
}

uvhttp_method_t uvhttp_method_from_string(const char* method) {
    return fast_method_parse(method);
}

const char* uvhttp_method_to_string(uvhttp_method_t method) {
    switch (method) {
    case UVHTTP_GET:
        return "GET";
    case UVHTTP_POST:
        return "POST";
    case UVHTTP_PUT:
        return "PUT";
    case UVHTTP_DELETE:
        return "DELETE";
    case UVHTTP_HEAD:
        return "HEAD";
    case UVHTTP_OPTIONS:
        return "OPTIONS";
    case UVHTTP_PATCH:
        return "PATCH";
    case UVHTTP_TRACE:
        return "TRACE";
    case UVHTTP_CONNECT:
        return "CONNECT";
    default:
        return "UNKNOWN";
    }
}

uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                              const char* prefix_path,
                                              void* static_context) {
    if (!router || !prefix_path || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 暂不支持静态文件路由 */
    return UVHTTP_ERROR_NOT_IMPLEMENTED;
}

uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                void* static_context) {
    if (!router || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 暂不支持回退路由 */
    return UVHTTP_ERROR_NOT_IMPLEMENTED;
}

#endif /* UVHTTP_FEATURE_ROUTER_CACHE */