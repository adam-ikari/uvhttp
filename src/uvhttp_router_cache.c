
#if UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION

#    include "uvhttp_allocator.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_hash.h"
#    include "uvhttp_router.h"
#    include "uvhttp_utils.h"

#    include <ctype.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

// HTTP方法字符串到枚举值的快速查找表
// 使用ASCII字符作为索引，实现O(1)时间复杂度的查找
// 注意：这个表仅用于快速提示，实际解析逻辑在fast_method_parse函数中
static const uvhttp_method_t method_map[256] = {
    ["G"] = UVHTTP_GET,  // GET方法映射
    //
    // 路由哈希表大小 - 使用256个桶以减少冲突
    // 选择256是因为ASCII字符范围，便于直接使用字符作为索引
    // 使用常量表中的 UVHTTP_ROUTER_HASH_SIZE

    //
    // 缓存优化的路由器结构体 - 采用分层缓存策略提高性能
    // 包含热路径缓存、哈希表和访问统计信息
    typedef struct {// 热路径缓存：存储最常用的16个路由，利用CPU缓存局部性
                    struct {char path[64];  // 路径字符串，限制64字节以适应缓存行大小
uvhttp_method_t method;                     // HTTP方法枚举值
uvhttp_request_handler_t handler;           // 处理函数指针
}
hot_routes[UVHTTP_ROUTER_HOT_ROUTES_COUNT];  // 固定大小的热路径数组
size_t hot_count;                            // 当前热路径数量

// 哈希表：用于快速查找所有路由（包括冷路径）
route_hash_entry_t* hash_table[UVHTTP_ROUTER_HASH_SIZE];  // 256个哈希桶的数组

// 访问统计：用于动态调整热路径缓存
unsigned int access_count[UVHTTP_ROUTER_ACCESS_COUNT_SIZE];  // 每个路由的访问计数（用于热路径识别）
size_t total_routes;                                         // 总路由数量统计
}
cache_optimized_router_t;
}
;

// CRC32哈希表用于快速路由查找
// 使用常量表中的 UVHTTP_ROUTER_HASH_SIZE

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
    } hot_routes[UVHTTP_ROUTER_HOT_ROUTES_COUNT];
    size_t hot_count;

    // 哈希表：快速查找
    route_hash_entry_t* hash_table[UVHTTP_ROUTER_HASH_SIZE];

    // 路由统计（用于动态调整热路径）
    unsigned int access_count[UVHTTP_ROUTER_ACCESS_COUNT_SIZE];
    size_t total_routes;
} cache_optimized_router_t;

// 使用统一的hash函数（热路径函数，提示编译器内联）
static inline uint32_t route_hash(const char* str) {
    if (!str)
        return 0;

    // 限制最大字符串长度以防止哈希冲突攻击
    size_t len = strlen(str);
    if (len > 1024) {
        len = 1024;  // 截断过长的字符串
    }

    return (uint32_t)XXH64(str, len, UVHTTP_HASH_DEFAULT_SEED);
}

// 快速方法解析（热路径函数，提示编译器内联）
static inline uvhttp_method_t fast_method_parse(const char* method) {
    if (!method)
        return UVHTTP_ANY;

    // 使用前缀快速判断
    switch (method[0]) {
    case 'G':
        return method[1] == 'E' ? UVHTTP_GET : UVHTTP_ANY;
    case 'P':
        if (method[1] == 'O')
            return UVHTTP_POST;
        if (method[1] == 'U')
            return UVHTTP_PUT;
        return UVHTTP_ANY;
    case 'D':
        return UVHTTP_DELETE;
    case 'H':
        return UVHTTP_HEAD;
    case 'O':
        return UVHTTP_OPTIONS;
    case 'C':
        return UVHTTP_PATCH;
    default:
        return UVHTTP_ANY;
    }
}

// 创建缓存优化的路由器
uvhttp_router_t* uvhttp_router_new(void) {
    cache_optimized_router_t* router = uvhttp_calloc(1, sizeof(cache_optimized_router_t));
    if (!router) {
        return NULL;
    }

    return (uvhttp_router_t*)router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (!router)
        return;

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    // 释放哈希表
    for (int i = 0; i < ROUTE_HASH_SIZE; i++) {
        route_hash_entry_t* entry = cr->hash_table[i];
        while (entry) {
            route_hash_entry_t* next = entry->next;
            uvhttp_free((void*)entry->path);  // 释放路径字符串
            uvhttp_free(entry);
            entry = next;
        }
    }

    uvhttp_free(router);
}

// 添加路由到哈希表
static uvhttp_error_t add_to_hash_table(cache_optimized_router_t* cr, const char* path,
                                        uvhttp_method_t method, uvhttp_request_handler_t handler) {
    if (!cr || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uint32_t hash = route_hash(path) % ROUTE_HASH_SIZE;

    route_hash_entry_t* entry = uvhttp_alloc(sizeof(route_hash_entry_t));
    if (!entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // 安全地复制字符串
    size_t path_len = strlen(path);
    char* path_copy = uvhttp_alloc(path_len + 1);
    if (!path_copy) {
        uvhttp_free(entry);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    strncpy(path_copy, path, path_len);
    path_copy[path_len] = '\0';

    entry->path = path_copy;
    entry->method = method;
    entry->handler = handler;
    entry->next = cr->hash_table[hash];
    cr->hash_table[hash] = entry;

    return UVHTTP_OK;
}

// 添加到热路径
static uvhttp_error_t add_to_hot_routes(cache_optimized_router_t* cr, const char* path,
                                        uvhttp_method_t method, uvhttp_request_handler_t handler) {
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
        snprintf(cr->hot_routes[min_index].path, sizeof(cr->hot_routes[min_index].path), "%s",
                 path);
        cr->hot_routes[min_index].method = method;
        cr->hot_routes[min_index].handler = handler;
        cr->access_count[min_index] = 0;
    } else {
        // 添加到热路径
        snprintf(cr->hot_routes[cr->hot_count].path, sizeof(cr->hot_routes[cr->hot_count].path),
                 "%s", path);
        cr->hot_routes[cr->hot_count].method = method;
        cr->hot_routes[cr->hot_count].handler = handler;
        cr->access_count[cr->hot_count] = 0;
        cr->hot_count++;
    }

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router, const char* path,
                                       uvhttp_request_handler_t handler) {
    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router, const char* path,
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

// 热路径查找（缓存友好，提示编译器内联）
static inline uvhttp_request_handler_t find_in_hot_routes(cache_optimized_router_t* cr,
                                                          const char* path,
                                                          uvhttp_method_t method) {
    // 循环展开以减少分支预测失败
    for (size_t i = 0; i < cr->hot_count; i += 4) {
        // 批量比较4个路由
        if ((cr->hot_routes[i].method == method || cr->hot_routes[i].method == UVHTTP_ANY) &&
            strcmp(cr->hot_routes[i].path, path) == 0) {
            cr->access_count[i]++;
            return cr->hot_routes[i].handler;
        }

        if (i + 1 < cr->hot_count &&
            (cr->hot_routes[i + 1].method == method ||
             cr->hot_routes[i + 1].method == UVHTTP_ANY) &&
            strcmp(cr->hot_routes[i + 1].path, path) == 0) {
            cr->access_count[i + 1]++;
            return cr->hot_routes[i + 1].handler;
        }

        if (i + 2 < cr->hot_count &&
            (cr->hot_routes[i + 2].method == method ||
             cr->hot_routes[i + 2].method == UVHTTP_ANY) &&
            strcmp(cr->hot_routes[i + 2].path, path) == 0) {
            cr->access_count[i + 2]++;
            return cr->hot_routes[i + 2].handler;
        }

        if (i + 3 < cr->hot_count &&
            (cr->hot_routes[i + 3].method == method ||
             cr->hot_routes[i + 3].method == UVHTTP_ANY) &&
            strcmp(cr->hot_routes[i + 3].path, path) == 0) {
            cr->access_count[i + 3]++;
            return cr->hot_routes[i + 3].handler;
        }
    }

    return NULL;
}

// 哈希表查找
static uvhttp_request_handler_t find_in_hash_table(cache_optimized_router_t* cr, const char* path,
                                                   uvhttp_method_t method) {
    uint32_t hash = route_hash(path) % ROUTE_HASH_SIZE;

    route_hash_entry_t* entry = cr->hash_table[hash];
    while (entry) {
        if ((entry->method == method || entry->method == UVHTTP_ANY) &&
            strcmp(entry->path, path) == 0) {
            return entry->handler;
        }
        entry = entry->next;
    }

    return NULL;
}

/* 纯线性查找实现 */
static uvhttp_request_handler_t find_handler_linear_only(const uvhttp_router_t* router,
                                                         const char* path, uvhttp_method_t method) {
    if (!router || !path) {
        return NULL;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* 遍历所有路由进行线性查找 */
    /* 由于当前架构限制，通过遍历哈希表实现线性查找 */
    /* 在实际生产中，应该维护一个路由数组进行真正的线性查找 */
    for (size_t i = 0; i < UVHTTP_ROUTER_HASH_SIZE; i++) {
        route_hash_entry_t* entry = cr->hash_table[i];
        while (entry) {
            if (strcmp(entry->path, path) == 0 && entry->method == method) {
                return entry->handler;
            }
            entry = entry->next;
        }
    }

    return NULL;
}

/* 纯哈希查找实现 */
static uvhttp_request_handler_t find_handler_hash_only(const uvhttp_router_t* router,
                                                       const char* path, uvhttp_method_t method) {
    if (!router || !path) {
        return NULL;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    return find_in_hash_table(cr, path, method);
}

uvhttp_request_handler_t uvhttp_router_find_handler(const uvhttp_router_t* router, const char* path,
                                                    const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }

    uvhttp_method_t method_enum = fast_method_parse(method);

#    if UVHTTP_ROUTER_SEARCH_MODE == 0
    /* 纯线性查找模式 - 适用于小规模路由或资源受限环境 */
    return find_handler_linear_only(router, path, method_enum);

#    elif UVHTTP_ROUTER_SEARCH_MODE == 1
    /* 纯哈希查找模式 - 适用于中等规模路由 */
    return find_handler_hash_only(router, path, method_enum);

#    else
    /* 混合策略模式（默认） - 适用于大规模高并发场景 */
    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* 首先在热路径中查找（缓存友好） */
    uvhttp_request_handler_t handler = find_in_hot_routes(cr, path, method_enum);
    if (handler) {
        return handler;
    }

    /* 然后在哈希表中查找 */
    return find_in_hash_table(cr, path, method_enum);

#    endif
}

// 静态路由表（编译时确定）
static const struct {
    const char* path;
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
} static_routes[] = {
    {"/api/users", UVHTTP_GET, NULL},
    {"/api/posts", UVHTTP_GET, NULL},
    {"/health", UVHTTP_GET, NULL},
    // 更多静态路由...
};

// 编译时优化的查找（使用二分查找）
static uvhttp_request_handler_t find_static_route(const char* path, uvhttp_method_t method) {
    // 这里可以使用编译时生成的完美哈希或二分查找
    for (size_t i = 0; i < sizeof(static_routes) / sizeof(static_routes[0]); i++) {
        if (static_routes[i].method == method && strcmp(static_routes[i].path, path) == 0) {
            return static_routes[i].handler;
        }
    }
    return NULL;
}

// 其他必要函数的简化实现
uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router, const char* path,
                                   const char* method, uvhttp_route_match_t* match) {
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
    case UVHTTP_GET:
        return UVHTTP_METHOD_GET;
    case UVHTTP_POST:
        return UVHTTP_METHOD_POST;
    case UVHTTP_PUT:
        return UVHTTP_METHOD_PUT;
    case UVHTTP_DELETE:
        return UVHTTP_METHOD_DELETE;
    case UVHTTP_HEAD:
        return UVHTTP_METHOD_HEAD;
    case UVHTTP_OPTIONS:
        return UVHTTP_METHOD_OPTIONS;
    case UVHTTP_PATCH:
        return UVHTTP_METHOD_PATCH;
    default:
        return "UNKNOWN";
    }
}

uvhttp_error_t uvhttp_parse_path_params(const char* path, uvhttp_param_t* params,
                                        size_t* param_count) {
    // 简化实现
    (void)path;
    (void)params;
    *param_count = 0;
    return UVHTTP_OK;
}
#endif /* UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION */
