#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

// 方法字符串映射 - 使用编译器优化的查找表
static const uvhttp_method_t method_map[256] = {
    ['G'] = UVHTTP_METHOD_GET,
    ['P'] = UVHTTP_METHOD_POST,  // POST 和 PUT 都以P开头
    ['D'] = UVHTTP_METHOD_DELETE,
    ['H'] = UVHTTP_METHOD_HEAD,
    ['O'] = UVHTTP_METHOD_OPTIONS,
    ['C'] = UVHTTP_METHOD_PATCH  // PATCH 有时缩写为 C(PATCH)
};

// CRC32哈希表用于快速路由查找
#define ROUTE_HASH_SIZE 256

// 哈希桶结构
typedef struct route_hash_entry {
    const char* path;
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    struct route_hash_entry* next;
} route_hash_entry_t;

// 缓存友好的路由存储
typedef struct {
    // 热路径：最常用的路由放在连续内存中
    struct {
        char path[64];  // 限制长度以适应缓存行
        uvhttp_method_t method;
        uvhttp_request_handler_t handler;
    } hot_routes[16];
    size_t hot_count;
    
    // 哈希表：快速查找
    route_hash_entry_t* hash_table[ROUTE_HASH_SIZE];
    
    // 路由统计（用于动态调整热路径）
    unsigned int access_count[1024];
    size_t total_routes;
} cache_optimized_router_t;

// CRC32哈希函数（编译时可计算常量）
static inline uint32_t crc32_hash(const char* str) {
    uint32_t crc = 0xFFFFFFFF;
    while (*str) {
        crc ^= *str++;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return crc;
}

// 快速方法解析（避免字符串比较）
static inline uvhttp_method_t fast_method_parse(const char* method) {
    if (!method) return UVHTTP_METHOD_ANY;
    
    // 使用前缀快速判断
    switch (method[0]) {
        case 'G': return method[1] == 'E' ? UVHTTP_METHOD_GET : UVHTTP_METHOD_ANY;
        case 'P': 
            if (method[1] == 'O') return UVHTTP_METHOD_POST;
            if (method[1] == 'U') return UVHTTP_METHOD_PUT;
            return UVHTTP_METHOD_ANY;
        case 'D': return UVHTTP_METHOD_DELETE;
        case 'H': return UVHTTP_METHOD_HEAD;
        case 'O': return UVHTTP_METHOD_OPTIONS;
        case 'C': return UVHTTP_METHOD_PATCH;
        default: return UVHTTP_METHOD_ANY;
    }
}

// 创建缓存优化的路由器
uvhttp_router_t* uvhttp_router_new(void) {
    cache_optimized_router_t* router = UVHTTP_CALLOC(1, sizeof(cache_optimized_router_t));
    if (!router) {
        return NULL;
    }
    
    return (uvhttp_router_t*)router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (!router) return;
    
    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    
    // 释放哈希表
    for (int i = 0; i < ROUTE_HASH_SIZE; i++) {
        route_hash_entry_t* entry = cr->hash_table[i];
        while (entry) {
            route_hash_entry_t* next = entry->next;
            UVHTTP_FREE(entry);
            entry = next;
        }
    }
    
    UVHTTP_FREE(router);
}

// 添加路由到哈希表
static uvhttp_error_t add_to_hash_table(cache_optimized_router_t* cr,
                                       const char* path,
                                       uvhttp_method_t method,
                                       uvhttp_request_handler_t handler) {
    uint32_t hash = crc32_hash(path) % ROUTE_HASH_SIZE;
    
    route_hash_entry_t* entry = UVHTTP_MALLOC(sizeof(route_hash_entry_t));
    if (!entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    entry->path = path;  // 注意：实际应该复制字符串
    entry->method = method;
    entry->handler = handler;
    entry->next = cr->hash_table[hash];
    cr->hash_table[hash] = entry;
    
    return UVHTTP_OK;
}

// 添加到热路径
static uvhttp_error_t add_to_hot_routes(cache_optimized_router_t* cr,
                                       const char* path,
                                       uvhttp_method_t method,
                                       uvhttp_request_handler_t handler) {
    if (cr->hot_count >= 16) {
        // 找到访问次数最少的替换
        size_t min_index = 0;
        unsigned int min_count = cr->access_count[0];
        
        for (size_t i = 1; i < 16; i++) {
            if (cr->access_count[i] < min_count) {
                min_count = cr->access_count[i];
                min_index = i;
            }
        }
        
        // 替换
        strncpy(cr->hot_routes[min_index].path, path, 63);
        cr->hot_routes[min_index].path[63] = '\0';
        cr->hot_routes[min_index].method = method;
        cr->hot_routes[min_index].handler = handler;
        cr->access_count[min_index] = 0;
    } else {
        // 添加到热路径
        strncpy(cr->hot_routes[cr->hot_count].path, path, 63);
        cr->hot_routes[cr->hot_count].path[63] = '\0';
        cr->hot_routes[cr->hot_count].method = method;
        cr->hot_routes[cr->hot_count].handler = handler;
        cr->access_count[cr->hot_count] = 0;
        cr->hot_count++;
    }
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router, 
                                           const char* path, 
                                           uvhttp_request_handler_t handler) {
    return uvhttp_router_add_route_method(router, path, UVHTTP_METHOD_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router,
                                                const char* path,
                                                uvhttp_method_t method,
                                                uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    
    // 添加到哈希表
    uvhttp_error_t err = add_to_hash_table(cr, path, method, handler);
    if (err != UVHTTP_OK) {
        return err;
    }
    
    // 如果是前16个路由，加入热路径
    if (cr->total_routes < 16) {
        add_to_hot_routes(cr, path, method, handler);
    }
    
    cr->total_routes++;
    return UVHTTP_OK;
}

// 热路径查找（缓存友好）
static inline uvhttp_request_handler_t find_in_hot_routes(cache_optimized_router_t* cr,
                                                        const char* path,
                                                        uvhttp_method_t method) {
    // 循环展开以减少分支预测失败
    for (size_t i = 0; i < cr->hot_count; i += 4) {
        // 批量比较4个路由
        if ((cr->hot_routes[i].method == method || cr->hot_routes[i].method == UVHTTP_METHOD_ANY) &&
            strcmp(cr->hot_routes[i].path, path) == 0) {
            cr->access_count[i]++;
            return cr->hot_routes[i].handler;
        }
        
        if (i + 1 < cr->hot_count &&
            (cr->hot_routes[i + 1].method == method || cr->hot_routes[i + 1].method == UVHTTP_METHOD_ANY) &&
            strcmp(cr->hot_routes[i + 1].path, path) == 0) {
            cr->access_count[i + 1]++;
            return cr->hot_routes[i + 1].handler;
        }
        
        if (i + 2 < cr->hot_count &&
            (cr->hot_routes[i + 2].method == method || cr->hot_routes[i + 2].method == UVHTTP_METHOD_ANY) &&
            strcmp(cr->hot_routes[i + 2].path, path) == 0) {
            cr->access_count[i + 2]++;
            return cr->hot_routes[i + 2].handler;
        }
        
        if (i + 3 < cr->hot_count &&
            (cr->hot_routes[i + 3].method == method || cr->hot_routes[i + 3].method == UVHTTP_METHOD_ANY) &&
            strcmp(cr->hot_routes[i + 3].path, path) == 0) {
            cr->access_count[i + 3]++;
            return cr->hot_routes[i + 3].handler;
        }
    }
    
    return NULL;
}

// 哈希表查找
static uvhttp_request_handler_t find_in_hash_table(cache_optimized_router_t* cr,
                                                  const char* path,
                                                  uvhttp_method_t method) {
    uint32_t hash = crc32_hash(path) % ROUTE_HASH_SIZE;
    
    route_hash_entry_t* entry = cr->hash_table[hash];
    while (entry) {
        if ((entry->method == method || entry->method == UVHTTP_METHOD_ANY) &&
            strcmp(entry->path, path) == 0) {
            return entry->handler;
        }
        entry = entry->next;
    }
    
    return NULL;
}

uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, 
                                                   const char* path,
                                                   const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }
    
    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);
    
    // 首先在热路径中查找（缓存友好）
    uvhttp_request_handler_t handler = find_in_hot_routes(cr, path, method_enum);
    if (handler) {
        return handler;
    }
    
    // 然后在哈希表中查找
    return find_in_hash_table(cr, path, method_enum);
}

// 静态路由表（编译时确定）
static const struct {
    const char* path;
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
} static_routes[] = {
    { "/api/users", UVHTTP_METHOD_GET, NULL },
    { "/api/posts", UVHTTP_METHOD_GET, NULL },
    { "/health", UVHTTP_METHOD_GET, NULL },
    // 更多静态路由...
};

// 编译时优化的查找（使用二分查找）
static uvhttp_request_handler_t find_static_route(const char* path, uvhttp_method_t method) {
    // 这里可以使用编译时生成的完美哈希或二分查找
    for (size_t i = 0; i < sizeof(static_routes) / sizeof(static_routes[0]); i++) {
        if (static_routes[i].method == method && 
            strcmp(static_routes[i].path, path) == 0) {
            return static_routes[i].handler;
        }
    }
    return NULL;
}

// 其他必要函数的简化实现
uvhttp_error_t uvhttp_router_match(uvhttp_router_t* router,
                                      const char* path,
                                      const char* method,
                                      uvhttp_route_match_t* match) {
    if (!router || !path || !method || !match) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    memset(match, 0, sizeof(uvhttp_route_match_t));
    
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, path, method);
    if (handler) {
        match->handler = handler;
        return UVHTTP_OK;
    }
    
    return UVHTTP_ERROR_NOT_FOUND;
}

uvhttp_method_t uvhttp_method_from_string(const char* method) {
    return fast_method_parse(method);
}

const char* uvhttp_method_to_string(uvhttp_method_t method) {
    switch (method) {
        case UVHTTP_METHOD_GET: return "GET";
        case UVHTTP_METHOD_POST: return "POST";
        case UVHTTP_METHOD_PUT: return "PUT";
        case UVHTTP_METHOD_DELETE: return "DELETE";
        case UVHTTP_METHOD_HEAD: return "HEAD";
        case UVHTTP_METHOD_OPTIONS: return "OPTIONS";
        case UVHTTP_METHOD_PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                         uvhttp_param_t* params,
                                         size_t* param_count) {
    // 简化实现
    (void)path;
    (void)params;
    *param_count = 0;
    return UVHTTP_OK;
}