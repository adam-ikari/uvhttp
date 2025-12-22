#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "include/uvhttp_utils.h"

// 简单的HTTP服务器测试
int main() {
    printf("Starting simple HTTP server test...\n");
    
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }
    
    // 绑定地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }
    
    // 开始监听
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    
    printf("Server listening on port 8080...\n");
    printf("Test our security functions...\n");
    
    // 测试安全函数
    char dest[10];
    int result = safe_strncpy(dest, "test", sizeof(dest));
    if (result != 0) {
        printf("FAIL: safe_strncpy\n");
        close(server_fd);
        return 1;
    }
    
    result = validate_url("/test", 5);
    if (result != 0) {
        printf("FAIL: validate_url\n");
        close(server_fd);
        return 1;
    }
    
    printf("Security tests passed!\n");
    printf("HTTP server socket created successfully.\n");
    printf("Basic functionality verified.\n");
    
    close(server_fd);
    return 0;
}