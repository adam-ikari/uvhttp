/**
 * @file test_main.c
 * @brief UVHTTP 测试主程序
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp.h"

/* 声明测试套件 */
extern int test_suite_utils(void);
extern int test_suite_response(void);
extern int test_suite_request(void);
extern int test_suite_router(void);

int main(int argc, char* argv[]) {
    int verbose = 0;
    
    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("UVHTTP 测试程序\n");
            printf("用法: %s [选项]\n", argv[0]);
            printf("选项:\n");
            printf("  -v, --verbose  详细输出\n");
            printf("  -h, --help     显示帮助信息\n");
            return 0;
        }
    }
    
    /* 初始化测试框架 */
    uvhttp_test_init(verbose);
    
    /* 安装内存跟踪器 */
    uvhttp_test_install_memory_tracker();
    
    /* 运行所有测试 */
    int failed_suites = uvhttp_test_run_all();
    
    /* 打印测试总结 */
    uvhttp_test_print_summary();
    
    /* 清理测试环境 */
    uvhttp_test_cleanup();
    
    return failed_suites;
}