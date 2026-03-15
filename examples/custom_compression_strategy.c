/**
 * @file custom_compression_strategy.c
 * @brief 演示如何使用 UVHTTP 提供的辅助函数实现自定义压缩策略
 * 
 * 这个示例展示了多种自定义压缩策略的实现方式：
 * 1. 使用辅助函数组合自定义策略
 * 2. 完全自定义压缩决策逻辑
 * 3. 基于请求动态调整压缩策略
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== 策略 1: 使用辅助函数组合自定义策略 ========== */

/**
 * @brief 自定义压缩策略 1: 基于文件扩展名和文件大小的组合策略
 * 
 * 策略规则:
 * - JSON 文件: 只要大于 1KB 就压缩
 * - 文本文件 (.txt, .md, .log, .csv): 只有大于 5KB 才压缩
 * - 其他文件: 不压缩
 */
static int custom_strategy_1(const char* filename, size_t file_size) {
    if (!filename) {
        return 0;
    }
    
    /* 检查是否是 JSON 文件 (优先检查) */
    if (strstr(filename, ".json")) {
        /* JSON 文件: 1KB 阈值 */
        return file_size >= 1024;
    }
    
    /* 检查是否是文本文件 */
    const char* text_extensions[] = {".txt", ".md", ".log", ".csv"};
    for (size_t i = 0; i < sizeof(text_extensions) / sizeof(text_extensions[0]); i++) {
        if (strstr(filename, text_extensions[i])) {
            /* 文本文件: 5KB 阈值 */
            return file_size >= 5120;
        }
    }
    
    /* 其他文件: 不压缩 */
    return 0;
}

/**
 * @brief 自定义压缩策略 2: 基于内容类型的动态策略
 * 
 * 策略规则:
 * - text/html: 总是压缩
 * - application/json: 大于 2KB 才压缩
 * - 其他: 不压缩
 */
static int custom_strategy_2(const char* content_type, size_t content_size) {
    if (!content_type) {
        return 0;
    }
    
    /* HTML 总是压缩 */
    if (strcasecmp(content_type, "text/html") == 0) {
        return 1;
    }
    
    /* JSON 大于 2KB 才压缩 */
    if (strcasecmp(content_type, "application/json") == 0) {
        return content_size >= 2048;
    }
    
    /* 其他情况使用 UVHTTP 的默认判断 */
    return uvhttp_should_compress_by_content_type(content_type) && content_size >= 1024;
}

/**
 * @brief 自定义压缩策略 3: 基于请求头和响应大小的策略
 * 
 * 策略规则:
 * - 客户端支持 gzip 且响应大于阈值时才压缩
 * - 阈值根据请求类型动态调整
 */
static int custom_strategy_3(uvhttp_request_t* request, size_t response_size) {
    if (!request) {
        return 0;
    }
    
    /* 检查客户端是否支持压缩 */
    const char* accept_encoding = uvhttp_request_get_header(request, "Accept-Encoding");
    if (!accept_encoding || !strstr(accept_encoding, "gzip")) {
        return 0;  /* 客户端不支持压缩 */
    }
    
    /* 根据请求路径动态调整阈值 */
    const char* path = uvhttp_request_get_path(request);
    size_t threshold = 1024;  /* 默认阈值 */
    
    if (strstr(path, "/api/")) {
        threshold = 512;  /* API 响应阈值更低 */
    } else if (strstr(path, "/static/")) {
        threshold = 2048;  /* 静态资源阈值更高 */
    }
    
    return response_size >= threshold;
}

/* ========== 示例处理器 ========== */

/**
 * @brief 使用策略 1 的处理器
 */
static int handler_strategy_1(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 生成较大的可压缩文本内容 (~8KB) */
    static char large_content[8192];
    static int content_initialized = 0;
    
    if (!content_initialized) {
        const char* pattern = "HTTP compression reduces network bandwidth. This is a highly compressible text pattern for testing compression effectiveness. ";
        size_t pattern_len = strlen(pattern);
        size_t pos = 0;
        while (pos + pattern_len < sizeof(large_content) - 1) {
            memcpy(large_content + pos, pattern, pattern_len);
            pos += pattern_len;
        }
        large_content[pos] = '\0';
        content_initialized = 1;
    }
    
    size_t content_len = strlen(large_content);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, large_content, content_len);
    
    /* 使用自定义策略 1 */
    if (custom_strategy_1("sample.txt", content_len)) {
        uvhttp_response_set_compress(response, 1);
        uvhttp_response_set_compress_threshold(response, 5120);
    }
    
    return uvhttp_response_send(response);
}

/**
 * @brief 使用策略 2 的处理器
 */
static int handler_strategy_2(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 生成较大的可压缩 JSON 内容 (~4KB) */
    static char json_content[4096];
    static int json_initialized = 0;
    
    if (!json_initialized) {
        int pos = 0;
        pos += snprintf(json_content + pos, sizeof(json_content) - pos, "{\"status\":\"ok\",\"items\":[");
        for (int i = 0; i < 200 && pos < (int)sizeof(json_content) - 100; i++) {
            pos += snprintf(json_content + pos, sizeof(json_content) - pos,
                           "{\"id\":%d,\"name\":\"Item %d\",\"description\":\"This is a test item for compression\",\"price\":19.99,\"in_stock\":true,\"category\":\"test\"},",
                           i, i);
        }
        if (pos > 2 && json_content[pos - 2] == ',') {
            pos -= 2;  /* Remove trailing comma */
            pos += snprintf(json_content + pos, sizeof(json_content) - pos, "],\"total\":200}");
        }
        json_initialized = 1;
    }
    
    size_t content_len = strlen(json_content);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, json_content, content_len);
    
    /* 使用自定义策略 2 */
    if (custom_strategy_2("application/json", content_len)) {
        uvhttp_response_set_compress(response, 1);
        uvhttp_response_set_compress_threshold(response, 2048);
    }
    
    return uvhttp_response_send(response);
}

/**
 * @brief 使用策略 3 的处理器（最灵活）
 */
static int handler_strategy_3(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 生成较大的可压缩内容 (~2KB) */
    static char content[2048];
    static int content_initialized = 0;
    
    if (!content_initialized) {
        const char* pattern = "This response uses strategy 3 based on client support. Compressing this text will demonstrate the effectiveness of custom compression strategies. ";
        size_t pattern_len = strlen(pattern);
        size_t pos = 0;
        while (pos + pattern_len < sizeof(content) - 1) {
            memcpy(content + pos, pattern, pattern_len);
            pos += pattern_len;
        }
        content[pos] = '\0';
        content_initialized = 1;
    }
    
    size_t content_len = strlen(content);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, content, content_len);
    
    /* 使用自定义策略 3（最灵活，考虑客户端支持） */
    if (custom_strategy_3(request, content_len)) {
        uvhttp_response_set_compress(response, 1);
    }
    
    return uvhttp_response_send(response);
}

/**
 * @brief 使用 UVHTTP 便捷函数的处理器
 */
static int handler_convenient(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 生成较大的 HTML 内容 (~3KB) */
    static char content[3072];
    static int content_initialized = 0;
    
    if (!content_initialized) {
        const char* pattern = "<div>This uses UVHTTP's convenient helper functions for automatic compression.</div>";
        size_t pattern_len = strlen(pattern);
        size_t pos = 0;
        while (pos + pattern_len < sizeof(content) - 1) {
            memcpy(content + pos, pattern, pattern_len);
            pos += pattern_len;
        }
        content[pos] = '\0';
        content_initialized = 1;
    }
    
    size_t content_len = strlen(content);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, content, content_len);
    
    /* 方式 1: 使用基于文件名的便捷函数 */
    // uvhttp_response_set_compress_by_filename(response, "index.html");
    
    /* 方式 2: 使用基于内容类型的便捷函数 */
    uvhttp_response_set_compress_by_content_type(response, "text/html");
    
    /* 方式 3: 手动组合辅助函数 */
    // if (uvhttp_should_compress_by_content_type("text/html")) {
    //     uvhttp_response_set_compress(response, 1);
    //     uvhttp_response_set_compress_threshold(response, 1024);
    // }
    
    return uvhttp_response_send(response);
}

/* ========== 主程序 ========== */

int main(int argc, char* argv[]) {
    int port = 18080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("========================================\n");
    printf("  UVHTTP 自定义压缩策略示例\n");
    printf("========================================\n");
    printf("Port: %d\n", port);
    printf("\n");
    printf("测试端点 (建议使用 curl -H 'Accept-Encoding: gzip' 测试):\n");
    printf("  GET /strategy1  - 自定义策略 1 (8KB 文本, 阈值 5KB)\n");
    printf("  GET /strategy2  - 自定义策略 2 (4KB JSON, 阈值 2KB)\n");
    printf("  GET /strategy3  - 自定义策略 3 (2KB 文本, 检查客户端支持)\n");
    printf("  GET /convenient - 便捷函数 (3KB HTML, 自动压缩)\n");
    printf("\n");
    printf("预期压缩效果:\n");
    printf("  strategy1: ~8KB -> ~1KB (文本高压缩率)\n");
    printf("  strategy2: ~4KB -> ~500B (JSON 高压缩率)\n");
    printf("  strategy3: ~2KB -> ~300B (客户端支持时压缩)\n");
    printf("  convenient: ~3KB -> ~400B (HTML 自动压缩)\n");
    printf("\n");
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建服务器 */
    uvhttp_server_t* server;
    if (uvhttp_server_new(loop, &server) != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    /* 创建路由 */
    uvhttp_router_t* router;
    if (uvhttp_router_new(&router) != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 注册路由 */
    uvhttp_router_add_route(router, "/strategy1", handler_strategy_1);
    uvhttp_router_add_route(router, "/strategy2", handler_strategy_2);
    uvhttp_router_add_route(router, "/strategy3", handler_strategy_3);
    uvhttp_router_add_route(router, "/convenient", handler_convenient);
    
    server->router = router;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server\n");
        uvhttp_router_free(router);
        uvhttp_server_free(server);
        return 1;
    }
    
    printf("Server started on http://0.0.0.0:%d\n", port);
    printf("Press Ctrl+C to stop\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_server_free(server);
    
    return 0;
}
