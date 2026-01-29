/* UVHTTP 异步文件读取模块 */

#ifndef UVHTTP_ASYNC_FILE_H
#define UVHTTP_ASYNC_FILE_H

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"

#    include <stddef.h>
#    include <time.h>
#    include <uv.h>

#    ifdef __cplusplus
extern "C" {
#    endif

/* 异步文件读取状态 */
typedef enum {
    UVHTTP_ASYNC_FILE_STATE_PENDING,   /* 等待读取 */
    UVHTTP_ASYNC_FILE_STATE_READING,   /* 正在读取 */
    UVHTTP_ASYNC_FILE_STATE_COMPLETED, /* 读取完成 */
    UVHTTP_ASYNC_FILE_STATE_ERROR      /* 读取错误 */
} uvhttp_async_file_state_t;

/* 异步文件读取请求 */
typedef struct uvhttp_async_file_request {
    uv_fs_t fs_req;                            /* libuv文件系统请求 */
    char file_path[UVHTTP_MAX_FILE_PATH_SIZE]; /* 文件路径 */
    char* buffer;                              /* 文件内容缓冲区 */
    size_t buffer_size;                        /* 缓冲区大小 */
    size_t file_size;                          /* 文件大小 */
    time_t last_modified;                      /* 最后修改时间 */
    uvhttp_async_file_state_t state;           /* 读取状态 */
    void* request;                             /* HTTP请求对象 */
    void* response;                            /* HTTP响应对象 */
    void* static_context;                      /* 静态文件上下文 */
    void (*completion_cb)(struct uvhttp_async_file_request* req,
                          int status);         /* 完成回调 */
    struct uvhttp_async_file_request* next;    /* 链表指针 */
    struct uvhttp_async_file_manager* manager; /* 管理器指针 */
} uvhttp_async_file_request_t;

/* 异步文件读取管理器 */
typedef struct uvhttp_async_file_manager {
    uv_loop_t* loop;                              /* 事件循环 */
    uvhttp_async_file_request_t* active_requests; /* 活跃请求链表 */
    int max_concurrent_reads;                     /* 最大并发读取数 */
    int current_reads;                            /* 当前读取数 */
    size_t read_buffer_size;                      /* 读取缓冲区大小 */
    size_t max_file_size;                         /* 最大文件大小 */
} uvhttp_async_file_manager_t;

/* 流式文件传输上下文 */
typedef struct uvhttp_file_stream_context {
    uv_fs_t fs_req;         /* 文件系统请求 */
    uv_file file_handle;    /* 文件句柄 */
    char* chunk_buffer;     /* 分块缓冲区 */
    size_t chunk_size;      /* 分块大小 */
    size_t file_offset;     /* 当前文件偏移 */
    size_t remaining_bytes; /* 剩余字节数 */
    void* response;         /* HTTP响应对象 */
    int is_active;          /* 是否活跃 */
} uvhttp_file_stream_context_t;

/**
 * 创建异步文件读取管理器
 *
 * @param loop 事件循环
 * @param max_concurrent 最大并发读取数
 * @param buffer_size 读取缓冲区大小
 * @param max_file_size 最大文件大小
 * @return 管理器指针，失败返回NULL
 */
uvhttp_error_t uvhttp_async_file_manager_create(
    uv_loop_t* loop, int max_concurrent, size_t buffer_size,
    size_t max_file_size, uvhttp_async_file_manager_t** manager);

/**
 * 释放异步文件读取管理器
 *
 * @param manager 管理器指针
 */
void uvhttp_async_file_manager_free(uvhttp_async_file_manager_t* manager);

/**
 * 异步读取文件
 *
 * @param manager 管理器
 * @param file_path 文件路径
 * @param request HTTP请求对象
 * @param response HTTP响应对象
 * @param static_context 静态文件上下文
 * @param completion_cb 完成回调
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_async_file_read(
    uvhttp_async_file_manager_t* manager, const char* file_path, void* request,
    void* response, void* static_context,
    void (*completion_cb)(uvhttp_async_file_request_t* req, int status));

/**
 * 取消异步文件读取
 *
 * @param manager 管理器
 * @param req 读取请求
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_async_file_cancel(uvhttp_async_file_manager_t* manager,
                                        uvhttp_async_file_request_t* req);

/**
 * 流式传输文件（适用于大文件）
 *
 * @param manager 管理器
 * @param file_path 文件路径
 * @param response HTTP响应对象
 * @param chunk_size 分块大小
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_async_file_stream(uvhttp_async_file_manager_t* manager,
                                        const char* file_path, void* response,
                                        size_t chunk_size);

/**
 * 停止文件流传输
 *
 * @param stream_ctx 流传输上下文
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_async_file_stream_stop(
    uvhttp_file_stream_context_t* stream_ctx);

/**
 * 获取管理器统计信息
 *
 * @param manager 管理器
 * @param current_reads 输出当前读取数
 * @param max_concurrent 输出最大并发数
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_async_file_get_stats(uvhttp_async_file_manager_t* manager,
                                           int* current_reads,
                                           int* max_concurrent);

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_FEATURE_STATIC_FILES */

#endif /* UVHTTP_ASYNC_FILE_H */