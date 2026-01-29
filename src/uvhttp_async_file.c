/* UVHTTP 异步文件读取模块实现 */

#if UVHTTP_FEATURE_STATIC_FILES

#include "uvhttp_async_file.h"
#include "uvhttp_static.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* 前向声明 */
static void on_file_read_complete(uv_fs_t* req);
static void on_file_stat_complete(uv_fs_t* req);
static void on_file_stream_chunk(uv_fs_t* req);
static void cleanup_async_request(uvhttp_async_file_request_t* req);
static void remove_request_from_manager(uvhttp_async_file_manager_t* manager, 
                                       uvhttp_async_file_request_t* req);

/**
 * 创建异步文件读取管理器
 */
uvhttp_error_t uvhttp_async_file_manager_create(uv_loop_t* loop,
                                                   int max_concurrent,
                                                   size_t buffer_size,
                                                   size_t max_file_size,
                                                   uvhttp_async_file_manager_t** manager) {
    if (!manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!loop || max_concurrent <= 0 || buffer_size == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_async_file_manager_t* mgr = uvhttp_alloc(sizeof(uvhttp_async_file_manager_t));
    if (!mgr) {
        uvhttp_handle_memory_failure("async_file_manager", NULL, NULL);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(mgr, 0, sizeof(uvhttp_async_file_manager_t));

    mgr->loop = loop;
    mgr->max_concurrent_reads = max_concurrent;
    mgr->current_reads = 0;
    mgr->read_buffer_size = buffer_size;
    mgr->max_file_size = max_file_size;
    mgr->active_requests = NULL;

    *manager = mgr;
    return UVHTTP_OK;
}

/**
 * 释放异步文件读取管理器
 */
void uvhttp_async_file_manager_free(uvhttp_async_file_manager_t* manager) {
    if (!manager) return;
    
    /* 取消所有活跃请求 */
    uvhttp_async_file_request_t* req = manager->active_requests;
    while (req) {
        uvhttp_async_file_request_t* next = req->next;
        uvhttp_async_file_cancel(manager, req);
        req = next;
    }
    
    uvhttp_free(manager);
}

/**
 * 从管理器中移除请求
 */
static void remove_request_from_manager(uvhttp_async_file_manager_t* manager, 
                                       uvhttp_async_file_request_t* req) {
    if (!manager || !req) return;
    
    if (manager->active_requests == req) {
        manager->active_requests = req->next;
    } else {
        uvhttp_async_file_request_t* current = manager->active_requests;
        while (current && current->next != req) {
            current = current->next;
        }
        if (current) {
            current->next = req->next;
        }
    }
    
    manager->current_reads--;
    req->next = NULL;
}

/**
 * 清理异步请求
 */
static void cleanup_async_request(uvhttp_async_file_request_t* req) {
    if (!req) return;
    
    if (req->buffer) {
        uvhttp_free(req->buffer);
        req->buffer = NULL;
    }
    
    if (req->fs_req.fs_type != UV_FS_UNKNOWN) {
        uv_fs_req_cleanup(&req->fs_req);
    }
    
    uvhttp_free(req);
}

/**
 * 文件读取完成回调
 */
static void on_file_read_complete(uv_fs_t* req) {
    uvhttp_async_file_request_t* async_req = (uvhttp_async_file_request_t*)req->data;
    if (!async_req) {
        uv_fs_req_cleanup(req);
        return;
    }
    
    uvhttp_async_file_manager_t* manager = async_req->manager;
    if (!manager) {
        cleanup_async_request(async_req);
        uv_fs_req_cleanup(req);
        return;
    }
    
    int status = -1;
    
    if (req->result >= 0) {
        /* 读取成功 */
        async_req->state = UVHTTP_ASYNC_FILE_STATE_COMPLETED;
        async_req->file_size = req->result;
        status = 0;
        
        /* 添加到缓存 */
        /* 注意：这里需要导入静态文件模块的缓存函数
         * 为了避免循环依赖，实际使用时需要在回调中处理缓存
         */
    } else {
        /* 读取失败 */
        async_req->state = UVHTTP_ASYNC_FILE_STATE_ERROR;
        uvhttp_log_safe_error(req->result, "async_file_read", async_req->file_path);
    }
    
    /* 关闭文件句柄 */
    if (req->fs_type == UV_FS_READ && req->result >= 0) {
        /* 文件描述符在fs_req中，需要关闭 */
        uv_fs_close(manager->loop, req, (uv_file)req->result, NULL);
    }
    
    /* 调用完成回调 */
    if (async_req->completion_cb) {
        async_req->completion_cb(async_req, status);
    }
    
    /* 从管理器中移除并清理 */
    remove_request_from_manager(manager, async_req);
    cleanup_async_request(async_req);
    uv_fs_req_cleanup(req);
}

/**
 * 文件状态获取完成回调
 */
static void on_file_stat_complete(uv_fs_t* req) {
    uvhttp_async_file_request_t* async_req = (uvhttp_async_file_request_t*)req->data;
    if (!async_req) {
        uv_fs_req_cleanup(req);
        return;
    }
    
    uvhttp_async_file_manager_t* manager = async_req->manager;
    if (!manager) {
        cleanup_async_request(async_req);
        uv_fs_req_cleanup(req);
        return;
    }

    if (req->result == 0) {
        /* stat成功 */
        struct stat* stat_buf = (struct stat*)req->ptr;
        if (stat_buf && S_ISREG(stat_buf->st_mode)) {
            /* 是常规文件 */
            if ((size_t)stat_buf->st_size <= manager->max_file_size) {
                async_req->file_size = stat_buf->st_size;
                async_req->last_modified = stat_buf->st_mtime;
                
                /* 分配缓冲区 */
                async_req->buffer = uvhttp_alloc(async_req->file_size);
                if (async_req->buffer) {
                    /* 开始异步读取 */
                    async_req->state = UVHTTP_ASYNC_FILE_STATE_READING;
                    
                    /* 需要重新打开文件进行读取 */
                    uv_fs_t open_req;
                    int ret = uv_fs_open(manager->loop, &open_req, async_req->file_path, 
                                        O_RDONLY, 0, NULL);
                    if (ret >= 0) {
                        uv_file file_fd = ret;
                        uv_fs_req_cleanup(&open_req);
                        
                        uv_buf_t buf = uv_buf_init(async_req->buffer, async_req->file_size);
                        ret = uv_fs_read(manager->loop, &async_req->fs_req, 
                                       file_fd, &buf, 1, 0, on_file_read_complete);
                        if (ret == 0) {
                            /* 保存文件描述符以便后续关闭 */
                            async_req->fs_req.result = file_fd;
                            return;  /* 等待读取完成 */
                        } else {
                            uvhttp_log_safe_error(ret, "async_file_read_start", async_req->file_path);
                            uv_fs_close(manager->loop, &open_req, file_fd, NULL);
                        }
                    } else {
                        uvhttp_log_safe_error(ret, "async_file_open", async_req->file_path);
                        uv_fs_req_cleanup(&open_req);
                    }
                } else {
                    uvhttp_handle_memory_failure("async_file_buffer", NULL, NULL);
                }
            } else {
                /* 文件过大 */
                uvhttp_log_safe_error(0, "async_file_too_large", async_req->file_path);
            }
        } else {
            /* 不是常规文件 */
            uvhttp_log_safe_error(0, "async_file_not_regular", async_req->file_path);
        }
    } else {
        /* stat失败 */
        uvhttp_log_safe_error(req->result, "async_file_stat", async_req->file_path);
    }
    
    /* 出错处理 */
    async_req->state = UVHTTP_ASYNC_FILE_STATE_ERROR;
    if (async_req->completion_cb) {
        int status = -1;
        async_req->completion_cb(async_req, status);
    }
    
    remove_request_from_manager(manager, async_req);
    cleanup_async_request(async_req);
    uv_fs_req_cleanup(req);
}

/**

 * 异步读取文件

 */



uvhttp_error_t uvhttp_async_file_read(uvhttp_async_file_manager_t* manager,

                                      const char* file_path,

                                      void* request,

                                      void* response,

                                      void* static_context,

                                      void (*completion_cb)(uvhttp_async_file_request_t* req, int status)) {

    if (!manager || !file_path || !request || !response || !completion_cb) {

        return UVHTTP_ERROR_INVALID_PARAM;

    }

    

    /* 检查并发限制 */

    if (manager->current_reads >= manager->max_concurrent_reads) {

        uvhttp_log_safe_error(0, "async_file_limit", "Too many concurrent reads");

        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;

    }

    

    /* 创建异步请求 */

    uvhttp_async_file_request_t* async_req = uvhttp_alloc(sizeof(uvhttp_async_file_request_t));

    if (!async_req) {

        uvhttp_handle_memory_failure("async_file_request", NULL, NULL);

        return UVHTTP_ERROR_OUT_OF_MEMORY;

    }

    

    memset(async_req, 0, sizeof(uvhttp_async_file_request_t));

    

    /* 设置请求参数 */

    strncpy(async_req->file_path, file_path, sizeof(async_req->file_path) - 1);

    async_req->file_path[sizeof(async_req->file_path) - 1] = '\0';

    async_req->request = request;
    async_req->response = response;
    async_req->static_context = static_context;
    async_req->completion_cb = completion_cb;
    async_req->state = UVHTTP_ASYNC_FILE_STATE_PENDING;
    async_req->fs_req.data = async_req;
    async_req->manager = manager;
    
    /* 添加到管理器 */
    async_req->next = manager->active_requests;
    manager->active_requests = async_req;
    manager->current_reads++;
    
    /* 开始异步stat操作 */
    int ret = uv_fs_stat(manager->loop, &async_req->fs_req, file_path, on_file_stat_complete);
    if (ret != 0) {
        /* 失败处理 */
        remove_request_from_manager(manager, async_req);
        cleanup_async_request(async_req);
        uvhttp_log_safe_error(ret, "async_file_stat_start", file_path);
        return UVHTTP_ERROR_IO_ERROR;
    }
    
    return UVHTTP_OK;
}

/**
 * 取消异步文件读取
 */
uvhttp_error_t uvhttp_async_file_cancel(uvhttp_async_file_manager_t* manager,
                                       uvhttp_async_file_request_t* req) {
    if (!manager || !req) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 取消文件系统操作 */
    if (req->fs_req.fs_type != UV_FS_UNKNOWN) {
        uv_cancel((uv_req_t*)&req->fs_req);
    }
    
    /* 从管理器中移除 */
    remove_request_from_manager(manager, req);
    
    /* 清理请求 */
    cleanup_async_request(req);
    
    return UVHTTP_OK;
}

/**
 * 文件流传输分块读取回调
 */
static void on_file_stream_chunk(uv_fs_t* req) {
    uvhttp_file_stream_context_t* stream_ctx = (uvhttp_file_stream_context_t*)req->data;
    if (!stream_ctx || !stream_ctx->is_active) {
        uv_fs_req_cleanup(req);
        return;
    }
    
    if (req->result > 0) {
        /* 读取成功，发送数据块 */
        /* 这里应该调用HTTP响应发送函数 */
        /* uvhttp_response_send_chunk(stream_ctx->response, stream_ctx->chunk_buffer, req->result); */
        
        /* 更新偏移 */
        stream_ctx->file_offset += req->result;
        stream_ctx->remaining_bytes -= req->result;
        
        /* 继续读取下一块 */
        if (stream_ctx->remaining_bytes > 0) {
            uv_buf_t buf = uv_buf_init(stream_ctx->chunk_buffer, stream_ctx->chunk_size);
            int ret = uv_fs_read(uv_default_loop(), &stream_ctx->fs_req,
                               stream_ctx->file_handle, &buf, 1, stream_ctx->file_offset,
                               on_file_stream_chunk);
            if (ret != 0) {
                /* 读取失败，停止流传输 */
                stream_ctx->is_active = 0;
                uv_fs_close(uv_default_loop(), &stream_ctx->fs_req, 
                          stream_ctx->file_handle, NULL);
            }
        } else {
            /* 文件传输完成 */
            stream_ctx->is_active = 0;
            uv_fs_close(uv_default_loop(), &stream_ctx->fs_req, 
                      stream_ctx->file_handle, NULL);
        }
    } else {
        /* 读取失败或EOF */
        stream_ctx->is_active = 0;
        uv_fs_close(uv_default_loop(), &stream_ctx->fs_req, 
                  stream_ctx->file_handle, NULL);
    }
    
    uv_fs_req_cleanup(req);
}

/**
 * 流式传输文件（适用于大文件）
 */
uvhttp_error_t uvhttp_async_file_stream(uvhttp_async_file_manager_t* manager,
                                       const char* file_path,
                                       void* response,
                                       size_t chunk_size) {
    if (!manager || !file_path || !response || chunk_size == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 创建流传输上下文 */
    uvhttp_file_stream_context_t* stream_ctx = uvhttp_alloc(sizeof(uvhttp_file_stream_context_t));
    if (!stream_ctx) {
        uvhttp_handle_memory_failure("file_stream_context", NULL, NULL);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    memset(stream_ctx, 0, sizeof(uvhttp_file_stream_context_t));
    
    stream_ctx->chunk_size = chunk_size;
    stream_ctx->response = response;
    stream_ctx->is_active = 1;
    
    /* 分配分块缓冲区 */
    stream_ctx->chunk_buffer = uvhttp_alloc(chunk_size);
    if (!stream_ctx->chunk_buffer) {
        uvhttp_free(stream_ctx);
        uvhttp_handle_memory_failure("file_stream_buffer", NULL, NULL);
        return -1;
    }
    
    /* 打开文件 */
    int ret = uv_fs_open(manager->loop, &stream_ctx->fs_req, file_path, 
                        O_RDONLY, 0, NULL);
    if (ret < 0) {
        uvhttp_free(stream_ctx->chunk_buffer);
        uvhttp_free(stream_ctx);
        uvhttp_log_safe_error(ret, "file_stream_open", file_path);
        return -1;
    }
    
    stream_ctx->file_handle = ret;
    stream_ctx->fs_req.data = stream_ctx;
    
    /* 获取文件大小 */
    uv_fs_t stat_req;
    if (uv_fs_stat(manager->loop, &stat_req, file_path, NULL) == 0) {
        struct stat* stat_buf = (struct stat*)stat_req.ptr;
        if (stat_buf) {
            stream_ctx->remaining_bytes = stat_buf->st_size;
        }
    }
    uv_fs_req_cleanup(&stat_req);
    
    /* 开始读取第一块 */
    uv_buf_t buf = uv_buf_init(stream_ctx->chunk_buffer, stream_ctx->chunk_size);
    ret = uv_fs_read(manager->loop, &stream_ctx->fs_req, stream_ctx->file_handle,
                    &buf, 1, stream_ctx->file_offset, on_file_stream_chunk);
    if (ret != 0) {
        uv_fs_close(manager->loop, &stream_ctx->fs_req, stream_ctx->file_handle, NULL);
        uvhttp_free(stream_ctx->chunk_buffer);
        uvhttp_free(stream_ctx);
        uvhttp_log_safe_error(ret, "file_stream_read_start", file_path);
        return UVHTTP_ERROR_IO_ERROR;
    }
    
    return UVHTTP_OK;
}

/**
 * 停止文件流传输
 */
uvhttp_error_t uvhttp_async_file_stream_stop(uvhttp_file_stream_context_t* stream_ctx) {
    if (!stream_ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    stream_ctx->is_active = 0;
    
    /* 取消文件系统操作 */
    if (stream_ctx->fs_req.fs_type != UV_FS_UNKNOWN) {
        uv_cancel((uv_req_t*)&stream_ctx->fs_req);
    }
    
    /* 关闭文件句柄 */
    if (stream_ctx->file_handle >= 0) {
        uv_fs_close(uv_default_loop(), &stream_ctx->fs_req, 
                  stream_ctx->file_handle, NULL);
    }
    
    /* 清理资源 */
    if (stream_ctx->chunk_buffer) {
        uvhttp_free(stream_ctx->chunk_buffer);
    }
    
    uvhttp_free(stream_ctx);
    
    return UVHTTP_OK;
}

/**
 * 获取管理器统计信息
 */
uvhttp_error_t uvhttp_async_file_get_stats(uvhttp_async_file_manager_t* manager,
                                         int* current_reads,
                                         int* max_concurrent) {
    if (!manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (current_reads) *current_reads = manager->current_reads;
    if (max_concurrent) *max_concurrent = manager->max_concurrent_reads;
    
    return UVHTTP_OK;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */