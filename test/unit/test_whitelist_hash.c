/*
 * 白名单哈希表功能测试
 * 
 * 测试目标：
 * 1. 验证白名单哈希表初始化
 * 2. 验证添加白名单IP
 * 3. 验证白名单查找功能
 * 4. 验证内存清理
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_server.h"

int main() {
    printf("=== 白名单哈希表功能测试 ===\n\n");
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "错误: 无法创建事件循环\n");
        return 1;
    }
    
    // 创建服务器
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        return 1;
    }
    
    printf("1. 测试哈希表初始化\n");
    assert(server->rate_limit_whitelist_hash == NULL);
    assert(server->rate_limit_whitelist_count == 0);
    printf("   ✓ 哈希表初始化正确\n\n");
    
    // 启用限流
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(server, 100, 60);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 无法启用限流功能\n");
        uvhttp_server_free(server);
        return 1;
    }
    
    printf("2. 测试添加白名单IP\n");
    const char* test_ips[] = {
        "192.168.1.1",
        "192.168.1.2",
        "192.168.1.3",
        "10.0.0.1",
        "172.16.0.1"
    };
    
    for (int i = 0; i < 5; i++) {
        result = uvhttp_server_add_rate_limit_whitelist(server, test_ips[i]);
        if (result != UVHTTP_OK) {
            fprintf(stderr, "错误: 无法添加白名单IP %s\n", test_ips[i]);
            uvhttp_server_free(server);
            return 1;
        }
        printf("   ✓ 添加IP: %s (哈希表大小: %zu, 数组大小: %zu)\n",
               test_ips[i],
               HASH_COUNT(server->rate_limit_whitelist_hash),
               server->rate_limit_whitelist_count);
    }
    assert(HASH_COUNT(server->rate_limit_whitelist_hash) == 5);
    assert(server->rate_limit_whitelist_count == 5);
    printf("   ✓ 所有IP添加成功\n\n");
    
    printf("3. 测试重复添加\n");
    result = uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.1");
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 重复添加返回错误\n");
        uvhttp_server_free(server);
        return 1;
    }
    assert(HASH_COUNT(server->rate_limit_whitelist_hash) == 5);
    assert(server->rate_limit_whitelist_count == 5);
    printf("   ✓ 重复添加被正确处理\n\n");
    
    printf("4. 测试哈希表查找功能\n");
    struct whitelist_item *item;
    HASH_FIND_STR(server->rate_limit_whitelist_hash, "192.168.1.1", item);
    assert(item != NULL);
    printf("   ✓ 找到IP: 192.168.1.1\n");
    
    HASH_FIND_STR(server->rate_limit_whitelist_hash, "192.168.1.99", item);
    assert(item == NULL);
    printf("   ✓ 正确未找到不存在的IP\n\n");
    
    printf("5. 测试NULL参数处理\n");
    result = uvhttp_server_add_rate_limit_whitelist(NULL, "192.168.1.1");
    assert(result == UVHTTP_ERROR_INVALID_PARAM);
    printf("   ✓ NULL服务器参数处理正确\n");
    
    result = uvhttp_server_add_rate_limit_whitelist(server, NULL);
    assert(result == UVHTTP_ERROR_INVALID_PARAM);
    printf("   ✓ NULL IP参数处理正确\n\n");
    
    printf("6. 测试内存清理\n");
    uvhttp_server_free(server);
    printf("   ✓ 服务器清理完成\n\n");
    
    printf("=== 所有测试通过 ===\n");
    return 0;
}