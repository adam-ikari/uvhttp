/*
 * 自定义文件类型 TTL 示例
 * 
 * 演示如何使用 uvhttp_static_set_file_ttl_map 函数
 * 为不同的文件类型设置自定义的缓存 TTL
 */

#include "uvhttp.h"
#include "uvhttp_static.h"
#include <stdio.h>
#include <stdlib.h>

int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 从 loop->data 获取 static_ctx 指针 */
    uvhttp_static_context_t** static_ctx_ptr = (uvhttp_static_context_t**)request->client->loop->data;
    if (!static_ctx_ptr || !*static_ctx_ptr) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static service not initialized", 30);
        uvhttp_response_send(response);
        return -1;
    }
    
    uvhttp_static_context_t* static_ctx = *static_ctx_ptr;
    
    /* 处理静态文件请求 */
    uvhttp_result_t result = uvhttp_static_handle_request(static_ctx, request, response);
    if (result != UVHTTP_OK) {
        uvhttp_response_set_status(response, 404);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Not Found", 9);
    }
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("=== 自定义文件类型 TTL 示例 ===\n");
    
    /* 创建测试文件 */
    system("mkdir -p ./public");
    FILE* f;
    
    /* 创建不同类型的测试文件 */
    f = fopen("./public/script.js", "w");
    if (f) {
        fprintf(f, "console.log('JavaScript file');");
        fclose(f);
    }
    
    f = fopen("./public/style.css", "w");
    if (f) {
        fprintf(f, "body { color: red; }");
        fclose(f);
    }
    
    f = fopen("./public/data.json", "w");
    if (f) {
        fprintf(f, "{\"key\": \"value\"}");
        fclose(f);
    }
    
    f = fopen("./public/index.html", "w");
    if (f) {
        fprintf(f, "<html><body>Custom TTL Demo</body></html>");
        fclose(f);
    }
    
    f = fopen("./public/image.png", "w");
    if (f) {
        fprintf(f, "PNG_DATA");
        fclose(f);
    }
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,
        .cache_ttl = 3600,
        .custom_headers = ""
    };
    
    /* 创建静态文件服务上下文 */
    uvhttp_static_context_t* static_ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &static_ctx);
    if (result != UVHTTP_OK) {
        printf("Error creating static context: %d\n", result);
        return 1;
    }
    
    /* 设置自定义文件类型 TTL 映射 */
    uvhttp_file_ttl_t ttl_map[] = {
        {".js", 7200},      /* JavaScript: 2小时 */
        {".css", 7200},     /* CSS: 2小时 */
        {".png", 86400},    /* PNG图片: 24小时 */
        {".jpg", 86400},    /* JPG图片: 24小时 */
        {".html", 120},     /* HTML: 2分钟 */
        {".json", 600},     /* JSON: 10分钟 */
        {".txt", 1800},     /* TXT: 30分钟 */
    };
    
    int map_size = sizeof(ttl_map) / sizeof(ttl_map[0]);
    result = uvhttp_static_set_file_ttl_map(static_ctx, ttl_map, map_size);
    if (result != UVHTTP_OK) {
        printf("Error setting file TTL map: %d\n", result);
        uvhttp_static_free(static_ctx);
        return 1;
    }
    
    printf("自定义 TTL 映射设置成功：\n");
    for (int i = 0; i < map_size; i++) {
        printf("  %s -> %d 秒\n", ttl_map[i].extension, ttl_map[i].ttl);
    }
    
    /* 创建服务器 */
    uv_loop_t* loop = uv_default_loop();
    
    /* 将 static_ctx 指针存储到 loop->data */
    loop->data = &static_ctx;
    
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    
    uvhttp_router_add_route(router, "/*", static_file_handler);
    
    server->router = router;
    
    if (uvhttp_server_listen(server, "0.0.0.0", 8085) != UVHTTP_OK) {
        printf("Failed to listen\n");
        uvhttp_static_free(static_ctx);
        return 1;
    }
    
    printf("\n 服务器启动成功！\n");
    printf("📍 服务地址: http://localhost:8085\n");
    printf("\n测试文件和自定义 TTL：\n");
    printf("  http://localhost:8085/script.js   - Cache-Control: max-age=7200\n");
    printf("  http://localhost:8085/style.css    - Cache-Control: max-age=7200\n");
    printf("  http://localhost:8085/image.png    - Cache-Control: max-age=86400\n");
    printf("  http://localhost:8085/index.html   - Cache-Control: max-age=120\n");
    printf("  http://localhost:8085/data.json    - Cache-Control: max-age=600\n");
    printf("\n使用 curl -I 查看响应头：\n");
    printf("  curl -I http://localhost:8085/script.js\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    uvhttp_static_free(static_ctx);
    uvhttp_server_free(server);
    
    printf("\n服务器已停止\n");
    return 0;
}