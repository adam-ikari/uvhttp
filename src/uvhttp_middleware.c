/*
 * UVHTTP 中间件系统实现
 *
 * 设计原则：
 * 1. 零开销抽象：使用内联函数和宏，避免不必要的开销
 * 2. 单线程模型：无锁设计，所有操作在事件循环线程中执行
 * 3. 高效执行：优先级排序，支持短路，最小化内存分配
 *
 * 执行流程：
 * 1. 中间件按优先级排序（HIGH > NORMAL > LOW）
 * 2. 每个中间件可以返回 0（继续）或非零（停止）
 * 3. 支持路径匹配（简单前缀匹配）
 * 4. 上下文数据在中间件生命周期内有效
 */

#include "uvhttp_middleware.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_error_helpers.h"
#include <stdlib.h>
#include <string.h>

/* 中间件返回值定义 */
#define UVHTTP_MIDDLEWARE_CONTINUE 0    /* 继续执行下一个中间件 */
#define UVHTTP_MIDDLEWARE_STOP 1        /* 停止执行中间件链 */

/**
 * 创建 HTTP 中间件
 * 
 * @param path 路径模式（NULL表示匹配所有路径）
 * @param handler 中间件处理函数
 * @param priority 优先级（HIGH > NORMAL > LOW）
 * @return 中间件对象，失败返回NULL
 * 
 * 零开销设计：
 * - 单次分配，包含所有字段
 * - 路径字符串使用 strdup 复制
 * - 初始化为零，避免未定义行为
 */
uvhttp_http_middleware_t* uvhttp_http_middleware_create(
    const char* path,
    uvhttp_http_middleware_handler_t handler,
    uvhttp_middleware_priority_t priority
) {
    /* 参数验证 */
    if (!handler) {
        /* Middleware handler cannot be NULL */
        return NULL;
    }
    
    /* 单次分配 - 零开销 */
    uvhttp_http_middleware_t* middleware = 
        (uvhttp_http_middleware_t*)uvhttp_alloc(sizeof(uvhttp_http_middleware_t));
    if (!middleware) {
        /* Memory allocation failed */
        return NULL;
    }
    
    /* 零初始化 - 确保所有字段都有确定值 */
    memset(middleware, 0, sizeof(uvhttp_http_middleware_t));
    
    /* 复制路径字符串（如果提供） */
    if (path) {
        size_t path_len = strlen(path);
        middleware->path = (char*)uvhttp_alloc(path_len + 1);
        if (!middleware->path) {
            /* Failed to allocate path */
            uvhttp_free(middleware);
            return NULL;
        }
        strcpy((char*)middleware->path, path);
    }
    
    /* 设置字段 */
    middleware->handler = handler;
    middleware->priority = priority;
    middleware->next = NULL;
    
    return middleware;
}

/**
 * 销毁 HTTP 中间件
 * 
 * @param middleware 中间件对象
 * 
 * 安全设计：
 * - 处理 NULL 输入
 * - 调用上下文清理函数
 * - 释放路径字符串
 * - 释放中间件结构
 */
void uvhttp_http_middleware_destroy(uvhttp_http_middleware_t* middleware) {
    if (!middleware) {
        return;
    }
    
    /* 清理上下文数据 */
    if (middleware->context.cleanup && middleware->context.data) {
        middleware->context.cleanup(middleware->context.data);
    }
    
    /* 释放路径字符串 */
    if (middleware->path) {
        uvhttp_free((char*)middleware->path);
    }
    
    /* 释放中间件结构 */
    uvhttp_free(middleware);
}

/**
 * 设置中间件上下文
 * 
 * @param middleware 中间件对象
 * @param data 上下文数据指针
 * @param cleanup 清理函数（可选）
 * 
 * 设计考虑：
 * - 允许中间件存储私有数据
 * - 提供清理机制防止内存泄漏
 * - 单线程安全，无需锁
 */
void uvhttp_http_middleware_set_context(
    uvhttp_http_middleware_t* middleware,
    void* data,
    void (*cleanup)(void*)
) {
    if (!middleware) {
        /* Cannot set context on NULL middleware */
        return;
    }
    
    middleware->context.data = data;
    middleware->context.cleanup = cleanup;
}

/**
 * 检查路径是否匹配中间件
 * 
 * @param middleware_path 中间件路径模式
 * @param request_path 请求路径
 * @return 1=匹配，0=不匹配
 * 
 * 匹配规则：
 * - NULL 路径匹配所有请求
 * - 精确匹配：路径完全相同
 * - 前缀匹配：请求路径以中间件路径开头
 * 
 * 零开销设计：
 * - 使用标准字符串函数
 * - 避免正则表达式开销
 * - 单次遍历
 */
static inline int middleware_path_match(
    const char* middleware_path,
    const char* request_path
) {
    /* NULL 路径匹配所有请求 */
    if (!middleware_path) {
        return 1;
    }
    
    /* 精确匹配 */
    if (strcmp(middleware_path, request_path) == 0) {
        return 1;
    }
    
    /* 前缀匹配 - 检查请求路径是否以中间件路径开头 */
    size_t mw_len = strlen(middleware_path);
    if (strncmp(middleware_path, request_path, mw_len) == 0) {
        /* 确保路径边界正确（后面是 / 或字符串结束） */
        if (request_path[mw_len] == '\0' || request_path[mw_len] == '/') {
            return 1;
        }
    }
    
    return 0;
}

/**
 * 执行 HTTP 中间件链
 * 
 * @param middleware 中间件链头
 * @param request HTTP 请求对象
 * @param response HTTP 响应对象
 * @return 0=继续执行路由，非零=停止执行
 * 
 * 执行流程：
 * 1. 遍历中间件链（已按优先级排序）
 * 2. 检查路径匹配
 * 3. 调用中间件处理函数
 * 4. 如果返回非零，停止执行
 * 5. 如果返回零，继续下一个中间件
 * 
 * 零开销设计：
 * - 直接遍历链表，无额外查找
 * - 路径匹配使用内联函数
 * - 支持短路（提前终止）
 * - 无动态内存分配
 */
int uvhttp_http_middleware_execute(
    uvhttp_http_middleware_t* middleware,
    uvhttp_request_t* request,
    uvhttp_response_t* response
) {
    /* 参数验证 */
    if (!middleware || !request || !response) {
        return UVHTTP_MIDDLEWARE_CONTINUE;
    }
    
    /* 获取请求路径 */
    const char* request_path = uvhttp_request_get_path(request);
    if (!request_path) {
        return UVHTTP_MIDDLEWARE_CONTINUE;
    }
    
    /* 遍历中间件链 */
    uvhttp_http_middleware_t* current = middleware;
    
    while (current) {
        /* 检查路径匹配 */
        if (middleware_path_match(current->path, request_path)) {
            /* 调用中间件处理函数 */
            int result = current->handler(request, response, &current->context);
            
            /* 如果返回非零，停止执行中间件链 */
            if (result != UVHTTP_MIDDLEWARE_CONTINUE) {
                return UVHTTP_MIDDLEWARE_STOP;
            }
        }
        
        /* 继续下一个中间件 */
        current = current->next;
    }
    
    /* 所有中间件执行完毕，继续路由分发 */
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/**
 * 向服务器添加中间件
 * 
 * @param server 服务器对象
 * @param middleware 中间件对象
 * @return UVHTTP_OK 成功，其他值失败
 * 
 * 插入策略：
 * - 按优先级插入（HIGH > NORMAL > LOW）
 * - 相同优先级：后添加的在后面
 * - 单线程安全，无需锁
 * 
 * 零开销设计：
 * - O(n) 插入（n 为中间件数量，通常很小）
 * - 直接指针操作，无额外分配
 */
uvhttp_error_t uvhttp_server_add_middleware(
    uvhttp_server_t* server,
    uvhttp_http_middleware_t* middleware
) {
    /* 参数验证 */
    if (!server || !middleware) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 空链表：直接添加 */
    if (!server->middleware_chain) {
        server->middleware_chain = middleware;
        return UVHTTP_OK;
    }
    
    /* 按优先级插入 */
    uvhttp_http_middleware_t* current = server->middleware_chain;
    uvhttp_http_middleware_t* prev = NULL;
    
    /* 找到插入位置 */
    while (current && current->priority >= middleware->priority) {
        prev = current;
        current = current->next;
    }
    
    /* 插入中间件 */
    if (prev) {
        /* 插入到中间 */
        middleware->next = current;
        prev->next = middleware;
    } else {
        /* 插入到头部 */
        middleware->next = server->middleware_chain;
        server->middleware_chain = middleware;
    }
    
    return UVHTTP_OK;
}

/**
 * 从服务器移除中间件
 * 
 * @param server 服务器对象
 * @param path 要移除的中间件路径
 * @return UVHTTP_OK 成功，其他值失败
 * 
 * 移除策略：
 * - 按路径匹配移除
 * - 只移除第一个匹配的中间件
 * - 单线程安全，无需锁
 * 
 * 零开销设计：
 * - O(n) 查找和移除
 * - 直接指针操作，无额外分配
 */
uvhttp_error_t uvhttp_server_remove_middleware(
    uvhttp_server_t* server,
    const char* path
) {
    /* 参数验证 */
    if (!server || !path) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 空链表 */
    if (!server->middleware_chain) {
        return UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND;
    }
    
    /* 查找并移除中间件 */
    uvhttp_http_middleware_t* current = server->middleware_chain;
    uvhttp_http_middleware_t* prev = NULL;
    
    while (current) {
        /* 检查路径匹配 */
        if (current->path && strcmp(current->path, path) == 0) {
            /* 从链表中移除 */
            if (prev) {
                prev->next = current->next;
            } else {
                server->middleware_chain = current->next;
            }
            
            /* 销毁中间件 */
            uvhttp_http_middleware_destroy(current);
            
            return UVHTTP_OK;
        }
        
        /* 继续查找 */
        prev = current;
        current = current->next;
    }
    
    /* 未找到 */
    return UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND;
}

/**
 * 清理服务器中间件链
 * 
 * @param server 服务器对象
 * 
 * 设计考虑：
 * - 在服务器销毁时调用
 * - 清理所有中间件及其上下文
 * - 单线程安全，无需锁
 * 
 * 零开销设计：
 * - O(n) 清理
 * - 直接指针操作
 */
void uvhttp_server_cleanup_middleware(uvhttp_server_t* server) {
    if (!server || !server->middleware_chain) {
        return;
    }
    
    /* 遍历并销毁所有中间件 */
    uvhttp_http_middleware_t* current = server->middleware_chain;
    
    while (current) {
        uvhttp_http_middleware_t* next = current->next;
        uvhttp_http_middleware_destroy(current);
        current = next;
    }
    
    /* 清空链表 */
    server->middleware_chain = NULL;
}