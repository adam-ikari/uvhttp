#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

// 示例1：使用内置内存池分配器
void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    
    // 使用自定义分配器分配内存
    char* message = uvhttp_malloc(128);
    if (message) {
        snprintf(message, 128, "Hello from memory pool allocator!");
        uvhttp_response_set_body(response, message, strlen(message));
        uvhttp_free(message);
    } else {
        uvhttp_response_set_body(response, "Memory allocation failed", 25);
    }
}

// 示例2：使用统计分配器监控内存使用
void stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_memory_stats_t stats;
    uvhttp_memory_stats_get(&stats);
    
    // 使用JSON响应显示内存统计
    char stats_json[512];
    snprintf(stats_json, sizeof(stats_json), 
        "{"
        "\"total_allocated\":%zu,"
        "\"current_usage\":%zu,"
        "\"peak_usage\":%zu,"
        "\"allocation_count\":%zu,"
        "\"free_count\":%zu"
        "}",
        stats.total_allocated,
        stats.current_usage,
        stats.peak_usage,
        stats.allocation_count,
        stats.free_count);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, stats_json, strlen(stats_json));
}

// 示例3：外部分配器适配
// 假设有一个第三方分配器
void* external_malloc(size_t size) {
    printf("External malloc: %zu bytes\n", size);
    return malloc(size);
}

void external_free(void* ptr) {
    printf("External free: %p\n", ptr);
    free(ptr);
}

void* external_realloc(void* ptr, size_t size) {
    printf("External realloc: %p -> %zu bytes\n", ptr, size);
    return realloc(ptr, size);
}

void* external_calloc(size_t nmemb, size_t size) {
    printf("External calloc: %zu x %zu\n", nmemb, size);
    return calloc(nmemb, size);
}

// 使用简化的外部分配器宏
UVHTTP_ALLOCATOR(external, external_malloc, external_free);

void external_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Using external allocator!", 28);
}

int main(int argc, char* argv[]) {
    printf("UVHTTP 内存分配器示例\n");
    
    // 根据命令行参数选择分配器类型
    uvhttp_allocator_type_t allocator_type = UVHTTP_ALLOCATOR_DEFAULT;
    
    if (argc > 1) {
        if (strcmp(argv[1], "pool") == 0) {
            allocator_type = UVHTTP_ALLOCATOR_POOL;
            printf("使用内存池分配器\n");
        } else if (strcmp(argv[1], "stats") == 0) {
            allocator_type = UVHTTP_ALLOCATOR_STATS;
            printf("使用统计分配器\n");
        } else if (strcmp(argv[1], "external") == 0) {
            printf("使用外部分配器\n");
        } else if (strcmp(argv[1], "mimalloc") == 0) {
            if (UVHTTP_HAS_MIMALLOC) {
                printf("使用mimalloc高性能分配器\n");
                uvhttp_mimalloc_init();
                allocator_type = UVHTTP_ALLOCATOR_CUSTOM; // 已设置
            } else {
                printf("mimalloc未启用，使用默认分配器\n");
            }
        } else {
            printf("使用默认分配器\n");
        }
    }
    
// 初始化分配器
    switch (allocator_type) {
        case UVHTTP_ALLOCATOR_POOL: {
            uvhttp_pool_config_t config = {
                .pool_size = 64 * 1024,
                .block_size = 256,
                .max_blocks = 256
            };
            uvhttp_allocator_t* pool_alloc = uvhttp_pool_allocator_new(&config);
            uvhttp_allocator_set(pool_alloc);
            break;
        }
        case UVHTTP_ALLOCATOR_STATS: {
            uvhttp_allocator_t* stats_alloc = uvhttp_stats_allocator_new(&g_default_allocator);
            uvhttp_allocator_set(stats_alloc);
            break;
        }
        case UVHTTP_ALLOCATOR_CUSTOM:
            if (strcmp(argv[1], "mimalloc") == 0) {
                if (UVHTTP_HAS_MIMALLOC) {
                    uvhttp_mimalloc_init();
                }
            } else {
                uvhttp_allocator_set(&external_allocator);
            }
            break;
        default:
            // 使用默认分配器
            break;
    }
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/stats", stats_handler);
    uvhttp_router_add_route(router, "/external", external_handler);
    
    server->router = router;
    
    // 启动服务器
    int ret = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (ret != 0) {
        fprintf(stderr, "启动失败: %s\n", uv_strerror(ret));
        uvhttp_allocator_cleanup();
        return 1;
    }
    
    printf("服务器运行在: http://localhost:8080\n");
    printf("访问:\n");
    printf("  http://localhost:8080/        - 主页\n");
    printf("  http://localhost:8080/stats    - 内存统计\n");
    printf("  http://localhost:8080/external - 外部分配器测试\n");
    printf("\n支持的分配器:\n");
    printf("  pool     - 内存池分配器\n");
    printf("  stats    - 统计分配器\n");
    printf("  external - 外部分配器\n");
    if (UVHTTP_HAS_MIMALLOC) {
        printf("  mimalloc  - mimalloc高性能分配器\n");
    }
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 显示最终内存统计
    if (allocator_type == UVHTTP_ALLOCATOR_STATS) {
        uvhttp_memory_stats_t final_stats;
        uvhttp_memory_stats_get(&final_stats);
        printf("\n最终内存统计:\n");
        printf("  总分配: %zu 字节\n", final_stats.total_allocated);
        printf("  当前使用: %zu 字节\n", final_stats.current_usage);
        printf("  峰值使用: %zu 字节\n", final_stats.peak_usage);
        printf("  分配次数: %zu\n", final_stats.allocation_count);
        printf("  释放次数: %zu\n", final_stats.free_count);
    }
    
    // 清理
    uvhttp_server_free(server);
    uv_loop_close(loop);
    free(loop);
    uvhttp_allocator_cleanup();
    
    return 0;
}