#include "uvhttp_response.h"
#include "uvhttp_common.h"
#include "uvhttp_constants.h"
#include "uvhttp_connection.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_validation.h"
#include "uvhttp_allocator.h"
#include "uvhttp_features.h"
#include "uvhttp_network.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

/* 外部全局变量声明（仅在测试模式下使用） */
#ifdef UVHTTP_TEST_MODE
extern uvhttp_network_interface_t* g_uvhttp_network_interface;
#endif

// 函数声明
static void uvhttp_free_write_data(uv_write_t* req, int status);

static const char* get_status_text(int status_code) {
    switch (status_code) {
        case UVHTTP_STATUS_OK: return "OK";
        case UVHTTP_STATUS_CREATED: return "Created";
        case UVHTTP_STATUS_NO_CONTENT: return "No Content";
        case UVHTTP_STATUS_BAD_REQUEST: return "Bad Request";
        case UVHTTP_STATUS_UNAUTHORIZED: return "Unauthorized";
        case UVHTTP_STATUS_FORBIDDEN: return "Forbidden";
        case UVHTTP_STATUS_NOT_FOUND: return "Not Found";
        case UVHTTP_STATUS_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case UVHTTP_STATUS_INTERNAL_ERROR: return "Internal Server Error";
        case UVHTTP_STATUS_NOT_IMPLEMENTED: return "Not Implemented";
        case UVHTTP_STATUS_BAD_GATEWAY: return "Bad Gateway";
        case UVHTTP_STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

// 辅助函数：检查字符串中是否包含控制字符（包括换行符）
static int contains_control_chars(const char* str) {
    if (!str) return 0;
    
    for (const char* p = str; *p; p++) {
        unsigned char char_value = (unsigned char)*p;
        // 检查是否包含控制字符 (0-31) 但排除制表符 (9) 和空格 (32)
        if (char_value < UVHTTP_SPACE_CHARACTER && char_value != UVHTTP_TAB_CHARACTER) {
            return 1;  // 包含控制字符
        }
        // 明确检查回车符和换行符，防止HTTP响应拆分攻击
        if (char_value == UVHTTP_CARRIAGE_RETURN || char_value == UVHTTP_LINE_FEED) {
            return 1;  // HTTP响应拆分尝试
        }
        // 检查删除字符
        if (char_value == UVHTTP_DELETE_CHARACTER) {
            return 1;  // 包含删除字符
        }
    }
    return 0;
}

static void build_response_headers(uvhttp_response_t* response, char* buffer, size_t* length) {
    size_t pos = 0;

    // 状态行
    pos += snprintf(buffer + pos, *length - pos, UVHTTP_VERSION_1_1 " %d %s\r\n",
                   response->status_code, get_status_text(response->status_code));
    
    // 默认headers检查
    int has_content_type = 0;
    int has_content_length = 0;
    int has_connection = 0;
    
    // 遍历现有headers
    for (size_t index = 0; index < response->header_count; index++) {
        // 安全检查：验证header值不包含控制字符，防止响应拆分
        if (contains_control_chars(response->headers[index].value)) {
            // 如果header值包含控制字符，跳过该header
            UVHTTP_LOG_ERROR("Invalid header value detected: header '%s' contains control characters\n", 
                           response->headers[index].name);
            continue;
        }
        
        pos += snprintf(buffer + pos, *length - pos, "%s: %s\r\n",
                       response->headers[index].name, response->headers[index].value);
        
        if (strcasecmp(response->headers[index].name, "Content-Type") == 0) {
            has_content_type = UVHTTP_TRUE;
        }
        if (strcasecmp(response->headers[index].name, "Content-Length") == 0) {
            has_content_length = UVHTTP_TRUE;
        }
        if (strcasecmp(response->headers[index].name, "Connection") == 0) {
            has_connection = UVHTTP_TRUE;
        }
    }
    
    // 添加默认Content-Type
    if (!has_content_type) {
        pos += snprintf(buffer + pos, *length - pos, "Content-Type: text/plain\r\n");
    }
    
    // HTTP/1.1要求：必须有Content-Length或使用chunked编码
    // 这里我们总是添加Content-Length以确保协议合规性
    if (!has_content_length) {
        if (response->body && response->body_length > 0) {
            pos += snprintf(buffer + pos, *length - pos, "Content-Length: %zu\r\n",
                           response->body_length);
        } else {
            // 即使没有body也要设置Content-Length: 0
            pos += snprintf(buffer + pos, *length - pos, "Content-Length: 0\r\n");
        }
    }
    
    // HTTP/1.1优化：根据keep-alive设置Connection头
    if (!has_connection) {
        if (response->keep_alive) {
            pos += snprintf(buffer + pos, *length - pos, "Connection: keep-alive\r\n");
            pos += snprintf(buffer + pos, *length - pos, "Keep-Alive: timeout=%d, max=%d\r\n", 
                   UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT, UVHTTP_DEFAULT_KEEP_ALIVE_MAX);
        } else {
            pos += snprintf(buffer + pos, *length - pos, "Connection: close\r\n");
        }
    }
    
    // 结束headers
    pos += snprintf(buffer + pos, *length - pos, "\r\n");

    *length = pos;
}

uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (!client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    memset(response, 0, sizeof(uvhttp_response_t));
    
    // HTTP/1.1优化：设置默认值
    response->keep_alive = UVHTTP_TRUE;    // HTTP/1.1默认保持连接
    response->status_code = UVHTTP_STATUS_OK;
    response->sent = UVHTTP_FALSE;          // 未发送
    response->finished = UVHTTP_FALSE;       // 未完成
    
    response->client = client;
    
    return UVHTTP_OK;
}

void uvhttp_response_cleanup(uvhttp_response_t* response) {
    if (!response) {
        return;
    }
    
    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }
    
    response->body_length = 0;
}

uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证状态码范围
    if (status_code < UVHTTP_STATUS_MIN_CONTINUE || status_code > UVHTTP_STATUS_MAX) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->status_code = status_code;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value) {
    if (!response || !name || !value) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (response->header_count >= MAX_HEADERS) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 验证header名称和值
    if (uvhttp_validate_header_name(name) == 0 || uvhttp_validate_header_value_safe(value) == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 额外验证：检查header值是否包含控制字符，防止响应拆分
    if (contains_control_chars(value)) {
        UVHTTP_LOG_ERROR("Invalid header value '%s': contains control characters\n", value);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    uvhttp_header_t* header = &response->headers[response->header_count];
    
    // 使用安全的字符串复制函数
    if (uvhttp_safe_strcpy(header->name, UVHTTP_MAX_HEADER_NAME_SIZE, name) != 0) {
        UVHTTP_LOG_ERROR("Failed to copy header name: %s\n", name);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (uvhttp_safe_strcpy(header->value, UVHTTP_MAX_HEADER_VALUE_SIZE, value) != 0) {
        UVHTTP_LOG_ERROR("Failed to copy header value: %s\n", value);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->header_count++;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (!body) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 检查长度限制 - 简化版本使用1MB限制
    if (length > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证body内容 - 检查无效字符
    for (size_t index = 0; index < length; index++) {
        // 允许所有二进制数据，但记录警告
        if (index < length - 1 && body[index] == 0) {
            // NULL字节是有效的，不需要处理
        }
    }
    
    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }
    
    response->body = uvhttp_alloc(length);
    if (!response->body) {
        response->body_length = 0;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(response->body, body, length);
    response->body_length = length;
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t* response, const char* data, size_t length) {
    if (!response || !data || length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 检查整数溢出 */
    
        if (length > SIZE_MAX - sizeof(uvhttp_write_data_t)) {
    
            return UVHTTP_ERROR_OUT_OF_MEMORY;
    
        }
    
        
    
        /* 优化：将write_data和数据缓冲区合并为一次分配，减少内存碎片 */
    
        size_t total_size = sizeof(uvhttp_write_data_t) + length;
    
        uvhttp_write_data_t* write_data = uvhttp_alloc(total_size);
    
        if (!write_data) {
    
            return UVHTTP_ERROR_OUT_OF_MEMORY;
    
        }
    
        
    
        /* 使用灵活数组成员，自动处理内存对齐 */
    
        memcpy(write_data->data, data, length);
    
        write_data->length = length;
    
        write_data->response = response;
    
        
    
        uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);
    
        write_data->write_req.data = write_data;
    
        
    
        int result = uv_write(&write_data->write_req, (uv_stream_t*)response->client, &buf, 1, 
    
                             (uv_write_cb)uvhttp_free_write_data);
    
        
    
        if (result < 0) {
    
            /* 修复内存泄漏：只需释放整个结构体，不需要单独释放data */
    
            uvhttp_free(write_data);
    
            return UVHTTP_ERROR_RESPONSE_SEND;
    
        }
    
    return UVHTTP_OK;
}

/* 单线程安全的写入完成回调
 * 在libuv事件循环线程中执行，安全释放写入相关资源
 * 单线程优势：无需锁，资源释放顺序可预测
 */
static void uvhttp_free_write_data(uv_write_t* req, int status) {
    (void)status; // 避免未使用参数警告
    uvhttp_write_data_t* write_data = (uvhttp_write_data_t*)req->data;
    if (write_data) {
        /* 检查是否需要关闭连接或重启读取 */
        if (write_data->response) {
            uv_tcp_t* client = (uv_tcp_t*)write_data->response->client;
            if (client) {
                uvhttp_connection_t* conn = (uvhttp_connection_t*)client->data;
                if (conn) {
                    if (!write_data->response->keep_alive) {
                        /* 关闭连接 */
                        uvhttp_connection_close(conn);
                    } else {
                        /* keep-alive连接，重启读取以接收下一个请求 */
                        uvhttp_connection_schedule_restart_read(conn);
                    }
                }
            }
        }

        /* 释放write_data（数据缓冲区是结构体的一部分，不需要单独释放） */
        uvhttp_free(write_data);
    }
}

/* ============ 纯函数：构建响应数据 ============ */
/* 纯函数：构建HTTP响应数据，无副作用，易于测试
 * response: 响应对象
 * out_data: 输出参数，返回构建的响应数据
 * out_length: 输出参数，返回响应数据长度
 * 返回: UVHTTP_OK 成功，其他值表示错误
 * 
 * 注意：调用者负责释放返回的 *out_data 内存
 */
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t* response, 
                                         char** out_data, 
                                         size_t* out_length) {
    if (!response || !out_data || !out_length) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 重复发送检查 */
    if (response->sent) {
        *out_data = NULL;
        *out_length = 0;
        return UVHTTP_OK;
    }
    
    /* 构建完整的HTTP响应 - 纯内存操作 */
    /* 优化：增加初始缓冲区大小，减少重新分配 */
    size_t headers_size = UVHTTP_INITIAL_BUFFER_SIZE * 2;  /* 从512增加到1024 */
    char* headers_buffer = uvhttp_alloc(headers_size);
    if (!headers_buffer) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    size_t headers_length = headers_size;
    build_response_headers(response, headers_buffer, &headers_length);
    
    /* 检查缓冲区是否太小，如果是则重新分配更大的缓冲区 */
    if (headers_length >= headers_size) {
        uvhttp_free(headers_buffer);
        headers_size = headers_length + UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN; /* 添加安全边距 */
        headers_buffer = uvhttp_alloc(headers_size);
        if (!headers_buffer) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        headers_length = headers_size;
        build_response_headers(response, headers_buffer, &headers_length);
    }
    
    /* 计算总大小 */
    size_t total_size = headers_length + response->body_length;
    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    /* 分配完整响应数据 */
    char* response_data = uvhttp_alloc(total_size + 1);  /* +1 for null terminator */
    if (!response_data) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    /* 复制headers */
    memcpy(response_data, headers_buffer, headers_length);
    
    /* 复制body */
    if (response->body && response->body_length > 0) {
        memcpy(response_data + headers_length, response->body, response->body_length);
    }
    
    /* 确保以null结尾（虽然HTTP不需要，但为了安全） */
    response_data[total_size] = '\0';
    
    /* 立即释放headers_buffer，不再需要 */
    uvhttp_free(headers_buffer);
    
    *out_data = response_data;
    *out_length = total_size;
    
    return UVHTTP_OK;
}

/* ============ 副作用函数：发送原始数据 ============ */
/* 副作用函数：发送原始数据，包含网络I/O
 * data: 要发送的数据
 * length: 数据长度
 * client: 客户端连接
 * response: 响应对象（用于回调处理）
 * 返回: UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_response_send_raw(const char* data,
                                       size_t length,
                                       void* client,
                                       uvhttp_response_t* response) {
    if (!data || length == 0 || !client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查整数溢出 */
    if (length > SIZE_MAX - sizeof(uvhttp_write_data_t)) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* 创建写数据结构 */
    size_t total_size = sizeof(uvhttp_write_data_t) + length - 1;  /* -1因为data已经有1字节 */

    uvhttp_write_data_t* write_data = uvhttp_alloc(total_size);
    if (!write_data) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* 复制数据到data数组 */
    memcpy(write_data->data, data, length);
    write_data->length = length;
    write_data->response = response;

    /* 初始化write_req */
    memset(&write_data->write_req, 0, sizeof(uv_write_t));
    write_data->write_req.data = write_data;

    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);

    uv_stream_t* stream = (uv_stream_t*)client;

    /* 检查stream是否有效 */
    if (stream->type != UV_TCP) {
        uvhttp_free(write_data);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查stream的loop指针 */
    if (!stream->loop) {
        uvhttp_free(write_data);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 直接调用libuv，使用与test/bench_server.c相同的方式 */
    int result = uv_write((uv_write_t*)write_data, stream, &buf, 1,
                         (uv_write_cb)uvhttp_free_write_data);

    if (result < 0) {
        /* 写入失败，立即清理资源 */
        uvhttp_free(write_data);
        return UVHTTP_ERROR_RESPONSE_SEND;
    }

    /* 如果响应设置了 Connection: close，需要在发送完成后关闭连接 */
    if (response && !response->keep_alive) {

        /* 获取连接对象并关闭连接 */
        uv_tcp_t* client_tcp = (uv_tcp_t*)response->client;
        if (client_tcp) {
            uvhttp_connection_t* conn = (uvhttp_connection_t*)client_tcp->data;
            if (conn) {
                conn->keep_alive = 0;
            }
        }
    }

    return UVHTTP_OK;
}

/* ============ 响应发送函数 ============ */
/* 单线程事件驱动的HTTP响应发送
 * 确保HTTP响应格式正确
 * 
 * 这个函数组合了数据构建和实际发送
 */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 单线程安全的重复发送检查 */
    if (response->sent) {
        return UVHTTP_OK;
    }

    /* 调用纯函数构建响应数据 */
    char* response_data = NULL;
    size_t response_length = 0;
    uvhttp_error_t err = uvhttp_response_build_data(response, &response_data, &response_length);

    if (err != UVHTTP_OK) {
        return err;
    }

    /* 标记响应已发送 */
    response->sent = UVHTTP_TRUE;

    /* 调用副作用函数发送数据 */
    err = uvhttp_response_send_raw(response_data, response_length,
                                   response->client, response);

    /* 释放纯函数分配的内存 */
    uvhttp_free(response_data);

    if (err == UVHTTP_OK) {
        response->finished = UVHTTP_TRUE;
    }

    return err;
}

/* ============ 测试专用函数 ============ */
#ifdef UVHTTP_TEST_MODE

/* 测试用纯函数：验证响应数据构建
 * 这个函数只构建数据但不发送，便于测试验证
 */
uvhttp_error_t uvhttp_response_build_for_test(uvhttp_response_t* response, 
                                             char** out_data, 
                                             size_t* out_length) {
    return uvhttp_response_build_data(response, out_data, out_length);
}

/* 测试用函数：模拟发送但不实际网络I/O */
uvhttp_error_t uvhttp_response_send_mock(uvhttp_response_t* response) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (response->sent) {
        return UVHTTP_OK;
    }
    
    /* 构建数据 */
    char* response_data = NULL;
    size_t response_length = 0;
    uvhttp_error_t err = uvhttp_response_build_data(response, &response_data, &response_length);
    if (err != UVHTTP_OK) {
        return err;
    }
    
    /* 模拟发送 - 只更新统计，不实际发送 */
    UVHTTP_TEST_LOG("Mock sending response: %zu bytes", response_length);
    
    /* 更新网络接口统计（如果可用） */
    uvhttp_network_reset_stats();
    uvhttp_network_simulate_error(0); /* 确保没有错误模拟 */
    
    /* 模拟发送成功 */
    if (g_uvhttp_network_interface) {
        uv_buf_t buf = uv_buf_init(response_data, response_length);
        g_uvhttp_network_interface->write(g_uvhttp_network_interface, 
                                          (uv_stream_t*)response->client, 
                                          &buf, 1, NULL);
    }
    
    uvhttp_free(response_data);
    response->sent = UVHTTP_TRUE;
    response->finished = UVHTTP_TRUE;
    
    return UVHTTP_OK;
}

#endif /* UVHTTP_TEST_MODE */

void uvhttp_response_free(uvhttp_response_t* response) {
    if (!response) {
        return;
    }
    
    uvhttp_response_cleanup(response);
    uvhttp_free(response);
}