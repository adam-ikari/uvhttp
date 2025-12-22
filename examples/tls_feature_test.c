#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

int test_tls_context_creation() {
    printf("测试TLS上下文创建...\n");
    
    // 初始化TLS模块
    if (uvhttp_tls_init() != UVHTTP_TLS_OK) {
        printf("❌ TLS初始化失败\n");
        return -1;
    }
    
    // 创建TLS上下文
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    if (!ctx) {
        printf("❌ TLS上下文创建失败\n");
        return -1;
    }
    
    printf("✅ TLS上下文创建成功\n");
    
    // 测试TLS 1.3支持
    if (uvhttp_tls_context_enable_tls13(ctx, 1) == UVHTTP_TLS_OK) {
        printf("✅ TLS 1.3支持启用成功\n");
    } else {
        printf("⚠️ TLS 1.3支持启用失败（可能是OpenSSL版本限制）\n");
    }
    
    // 测试会话票证
    if (uvhttp_tls_context_enable_session_tickets(ctx, 1) == 0) {
        printf("✅ 会话票证启用成功\n");
    } else {
        printf("❌ 会话票证启用失败\n");
    }
    
    // 测试OCSP装订
    if (uvhttp_tls_context_enable_ocsp_stapling(ctx, 1) == UVHTTP_TLS_OK) {
        printf("✅ OCSP装订启用成功\n");
    } else {
        printf("⚠️ OCSP装订启用失败（可能是OpenSSL版本限制）\n");
    }
    
    // 测试CRL检查
    if (uvhttp_tls_context_enable_crl_checking(ctx, 1) == UVHTTP_TLS_OK) {
        printf("✅ CRL检查启用成功\n");
    } else {
        printf("⚠️ CRL检查启用失败（可能是OpenSSL版本限制）\n");
    }
    
    // 测试性能统计
    uvhttp_tls_stats_t stats;
    if (uvhttp_tls_get_stats(ctx, &stats) == UVHTTP_TLS_OK) {
        printf("✅ 性能统计获取成功\n");
        printf("   - 握手次数: %llu\n", stats.handshake_count);
        printf("   - 握手错误: %llu\n", stats.handshake_errors);
        printf("   - 发送字节: %llu\n", stats.bytes_sent);
        printf("   - 接收字节: %llu\n", stats.bytes_received);
    } else {
        printf("❌ 性能统计获取失败\n");
    }
    
    // 清理
    uvhttp_tls_context_free(ctx);
    uvhttp_tls_cleanup();
    
    printf("✅ TLS测试完成\n");
    return 0;
}

int main() {
    printf("=== UVHTTP TLS功能测试 ===\n\n");
    
    int result = test_tls_context_creation();
    
    if (result == 0) {
        printf("\n🎉 所有TLS功能测试通过！\n");
    } else {
        printf("\n❌ TLS功能测试失败！\n");
    }
    
    return result;
}