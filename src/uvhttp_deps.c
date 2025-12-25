/* UVHTTP - 依赖注入框架实现 */

#include "uvhttp_deps.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <sys/socket.h>
#include <unistd.h>

/* ========== 默认循环提供者实现 ========== */

static uv_loop_t* default_get_default_loop(void* user_data) {
    (void)user_data;
    return uv_default_loop();
}

static uv_loop_t* default_create_loop(void* user_data) {
    (void)user_data;
    uv_loop_t* loop = uvhttp_malloc(sizeof(uv_loop_t));
    if (loop) {
        uv_loop_init(loop);
    }
    return loop;
}

static int default_run_loop(uv_loop_t* loop, uv_run_mode mode, void* user_data) {
    (void)user_data;
    return uv_run(loop, mode);
}

static void default_close_loop(uv_loop_t* loop, void* user_data) {
    (void)user_data;
    if (loop) {
        uv_loop_close(loop);
        free(loop);
    }
}

uvhttp_loop_provider_t* uvhttp_default_loop_provider_new(void) {
    uvhttp_loop_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_loop_provider_t));
    if (!provider) return NULL;
    
    provider->get_default_loop = default_get_default_loop;
    provider->create_loop = default_create_loop;
    provider->run_loop = default_run_loop;
    provider->close_loop = default_close_loop;
    provider->user_data = NULL;
    
    return provider;
}

/* ========== 默认内存提供者实现 ========== */

static void* default_malloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void* default_calloc(size_t count, size_t size, void* user_data) {
    (void)user_data;
    return calloc(count, size);
}

static void* default_realloc(void* ptr, size_t size, void* user_data) {
    (void)user_data;
    return realloc(ptr, size);
}

static void default_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

static size_t default_get_allocated_size(void* ptr, void* user_data) {
    (void)user_data;
    (void)ptr;
    return 0; /* 标准malloc无法获取大小 */
}

uvhttp_memory_provider_t* uvhttp_default_memory_provider_new(void) {
    uvhttp_memory_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_memory_provider_t));
    if (!provider) return NULL;
    
    provider->malloc = default_malloc;
    provider->calloc = default_calloc;
    provider->realloc = default_realloc;
    provider->free = default_free;
    provider->get_allocated_size = default_get_allocated_size;
    provider->user_data = NULL;
    
    return provider;
}

/* ========== 默认网络提供者实现 ========== */

static int default_create_socket(int domain, int type, int protocol, void* user_data) {
    (void)user_data;
    return socket(domain, type, protocol);
}

static int default_bind_socket(int sockfd, const struct sockaddr* addr, socklen_t addrlen, void* user_data) {
    (void)user_data;
    return bind(sockfd, addr, addrlen);
}

static int default_listen_socket(int sockfd, int backlog, void* user_data) {
    (void)user_data;
    return listen(sockfd, backlog);
}

static int default_accept_socket(int sockfd, struct sockaddr* addr, socklen_t* addrlen, void* user_data) {
    (void)user_data;
    return accept(sockfd, addr, addrlen);
}

static ssize_t default_send_data(int sockfd, const void* buf, size_t len, int flags, void* user_data) {
    (void)user_data;
    return send(sockfd, buf, len, flags);
}

static ssize_t default_recv_data(int sockfd, void* buf, size_t len, int flags, void* user_data) {
    (void)user_data;
    return recv(sockfd, buf, len, flags);
}

static void default_close_socket(int sockfd, void* user_data) {
    (void)user_data;
    close(sockfd);
}

uvhttp_network_provider_t* uvhttp_default_network_provider_new(void) {
    uvhttp_network_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_network_provider_t));
    if (!provider) return NULL;
    
    provider->create_socket = default_create_socket;
    provider->bind_socket = default_bind_socket;
    provider->listen_socket = default_listen_socket;
    provider->accept_socket = default_accept_socket;
    provider->send_data = default_send_data;
    provider->recv_data = default_recv_data;
    provider->close_socket = default_close_socket;
    provider->user_data = NULL;
    
    return provider;
}

/* ========== 默认文件提供者实现 ========== */

static FILE* default_fopen(const char* filename, const char* mode, void* user_data) {
    (void)user_data;
    return fopen(filename, mode);
}

static int default_fclose(FILE* stream, void* user_data) {
    (void)user_data;
    return fclose(stream);
}

static size_t default_fread(void* ptr, size_t size, size_t count, FILE* stream, void* user_data) {
    (void)user_data;
    return fread(ptr, size, count, stream);
}

static size_t default_fwrite(const void* ptr, size_t size, size_t count, FILE* stream, void* user_data) {
    (void)user_data;
    return fwrite(ptr, size, count, stream);
}

static int default_fseek(FILE* stream, long offset, int whence, void* user_data) {
    (void)user_data;
    return fseek(stream, offset, whence);
}

static long default_ftell(FILE* stream, void* user_data) {
    (void)user_data;
    return ftell(stream);
}

static int default_access(const char* pathname, int mode, void* user_data) {
    (void)user_data;
    return access(pathname, mode);
}

uvhttp_file_provider_t* uvhttp_default_file_provider_new(void) {
    uvhttp_file_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_file_provider_t));
    if (!provider) return NULL;
    
    provider->fopen = default_fopen;
    provider->fclose = default_fclose;
    provider->fread = default_fread;
    provider->fwrite = default_fwrite;
    provider->fseek = default_fseek;
    provider->ftell = default_ftell;
    provider->access = default_access;
    provider->user_data = NULL;
    
    return provider;
}

/* ========== 测试内存提供者实现（支持泄漏检测） ========== */

typedef struct {
    size_t total_allocated;
    size_t allocation_count;
    bool track_allocations;
} test_memory_data_t;

static void* test_malloc(size_t size, void* user_data) {
    test_memory_data_t* data = (test_memory_data_t*)user_data;
    void* ptr = malloc(size);
    if (ptr && data->track_allocations) {
        data->total_allocated += size;
        data->allocation_count++;
    }
    return ptr;
}

static void* test_calloc(size_t count, size_t size, void* user_data) {
    test_memory_data_t* data = (test_memory_data_t*)user_data;
    void* ptr = calloc(count, size);
    if (ptr && data->track_allocations) {
        data->total_allocated += count * size;
        data->allocation_count++;
    }
    return ptr;
}

static void test_free(void* ptr, void* user_data) {
    test_memory_data_t* data = (test_memory_data_t*)user_data;
    if (ptr && data->track_allocations) {
        data->allocation_count--;
    }
    free(ptr);
}

static size_t test_get_allocated_size(void* ptr, void* user_data) {
    (void)ptr;
    test_memory_data_t* data = (test_memory_data_t*)user_data;
    return data->total_allocated;
}

uvhttp_memory_provider_t* uvhttp_test_memory_provider_new(void) {
    test_memory_data_t* data = uvhttp_malloc(sizeof(test_memory_data_t));
    if (!data) return NULL;
    
    memset(data, 0, sizeof(test_memory_data_t));
    data->track_allocations = true;
    
    uvhttp_memory_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_memory_provider_t));
    if (!provider) {
        uvhttp_free(data);
        return NULL;
    }
    
    provider->malloc = test_malloc;
    provider->calloc = test_calloc;
    provider->realloc = default_realloc; /* 复用默认实现 */
    provider->free = test_free;
    provider->get_allocated_size = test_get_allocated_size;
    provider->user_data = data;
    
    return provider;
}

/* ========== 测试网络提供者实现（支持Mock） ========== */

typedef struct {
    int mock_socket_result;
    ssize_t mock_send_result;
    ssize_t mock_recv_result;
    bool mock_enabled;
    char* recv_buffer;
    size_t recv_buffer_size;
} test_network_data_t;

static int test_create_socket(int domain, int type, int protocol, void* user_data) {
    test_network_data_t* data = (test_network_data_t*)user_data;
    if (data->mock_enabled) {
        return data->mock_socket_result;
    }
    return default_create_socket(domain, type, protocol, NULL);
}

static ssize_t test_send_data(int sockfd, const void* buf, size_t len, int flags, void* user_data) {
    test_network_data_t* data = (test_network_data_t*)user_data;
    if (data->mock_enabled) {
        return data->mock_send_result;
    }
    return default_send_data(sockfd, buf, len, flags, NULL);
}

static ssize_t test_recv_data(int sockfd, void* buf, size_t len, int flags, void* user_data) {
    test_network_data_t* data = (test_network_data_t*)user_data;
    if (data->mock_enabled && data->recv_buffer) {
        size_t copy_len = len < data->recv_buffer_size ? len : data->recv_buffer_size;
        memcpy(buf, data->recv_buffer, copy_len);
        return copy_len;
    }
    return default_recv_data(sockfd, buf, len, flags, NULL);
}

uvhttp_network_provider_t* uvhttp_test_network_provider_new(void) {
    test_network_data_t* data = uvhttp_malloc(sizeof(test_network_data_t));
    if (!data) return NULL;
    
    memset(data, 0, sizeof(test_network_data_t));
    data->mock_socket_result = -1; /* 默认失败 */
    
    uvhttp_network_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_network_provider_t));
    if (!provider) {
        uvhttp_free(data);
        return NULL;
    }
    
    provider->create_socket = test_create_socket;
    provider->bind_socket = default_bind_socket; /* 复用默认实现 */
    provider->listen_socket = default_listen_socket; /* 复用默认实现 */
    provider->accept_socket = default_accept_socket; /* 复用默认实现 */
    provider->send_data = test_send_data;
    provider->recv_data = test_recv_data;
    provider->close_socket = default_close_socket; /* 复用默认实现 */
    provider->user_data = data;
    
    return provider;
}

/* ========== 测试文件提供者实现（支持虚拟文件系统） ========== */

typedef struct {
    char** virtual_files;
    char** virtual_content;
    size_t* virtual_sizes;
    size_t virtual_count;
    bool virtual_enabled;
} test_file_data_t;

static FILE* test_fopen(const char* filename, const char* mode, void* user_data) {
    test_file_data_t* data = (test_file_data_t*)user_data;
    if (data->virtual_enabled) {
        /* 在虚拟文件系统中查找文件 */
        for (size_t i = 0; i < data->virtual_count; i++) {
            if (strcmp(data->virtual_files[i], filename) == 0) {
                /* 创建一个内存文件流 */
                return fmemopen(data->virtual_content[i], data->virtual_sizes[i], mode);
            }
        }
        return NULL; /* 文件不存在 */
    }
    return default_fopen(filename, mode, NULL);
}

static int test_access(const char* pathname, int mode, void* user_data) {
    test_file_data_t* data = (test_file_data_t*)user_data;
    if (data->virtual_enabled) {
        /* 在虚拟文件系统中检查文件存在性 */
        for (size_t i = 0; i < data->virtual_count; i++) {
            if (strcmp(data->virtual_files[i], pathname) == 0) {
                return 0; /* 文件存在 */
            }
        }
        return -1; /* 文件不存在 */
    }
    return default_access(pathname, mode, NULL);
}

uvhttp_file_provider_t* uvhttp_test_file_provider_new(void) {
    test_file_data_t* data = uvhttp_malloc(sizeof(test_file_data_t));
    if (!data) return NULL;
    
    memset(data, 0, sizeof(test_file_data_t));
    
    uvhttp_file_provider_t* provider = uvhttp_malloc(sizeof(uvhttp_file_provider_t));
    if (!provider) {
        uvhttp_free(data);
        return NULL;
    }
    
    provider->fopen = test_fopen;
    provider->fclose = default_fclose; /* 复用默认实现 */
    provider->fread = default_fread; /* 复用默认实现 */
    provider->fwrite = default_fwrite; /* 复用默认实现 */
    provider->fseek = default_fseek; /* 复用默认实现 */
    provider->ftell = default_ftell; /* 复用默认实现 */
    provider->access = test_access;
    provider->user_data = data;
    
    return provider;
}

/* ========== 依赖容器管理实现 ========== */

static void default_cleanup(uvhttp_deps_t* deps) {
    if (!deps) return;
    
    if (deps->owns_providers) {
        if (deps->loop_provider) {
            uvhttp_free(deps->loop_provider);
        }
        if (deps->memory_provider) {
            uvhttp_free(deps->memory_provider->user_data);
            uvhttp_free(deps->memory_provider);
        }
        if (deps->network_provider) {
            uvhttp_free(deps->network_provider->user_data);
            uvhttp_free(deps->network_provider);
        }
        if (deps->file_provider) {
            uvhttp_free(deps->file_provider->user_data);
            uvhttp_free(deps->file_provider);
        }
    }
}

uvhttp_deps_t* uvhttp_deps_new(void) {
    uvhttp_deps_t* deps = uvhttp_malloc(sizeof(uvhttp_deps_t));
    if (!deps) return NULL;
    
    memset(deps, 0, sizeof(uvhttp_deps_t));
    deps->cleanup = default_cleanup;
    
    return deps;
}

uvhttp_result_t uvhttp_deps_set_loop_provider(uvhttp_deps_t* deps, uvhttp_loop_provider_t* provider) {
    if (!deps) return UVHTTP_ERROR_INVALID_PARAM;
    deps->loop_provider = provider;
    return UVHTTP_OK;
}

uvhttp_result_t uvhttp_deps_set_memory_provider(uvhttp_deps_t* deps, uvhttp_memory_provider_t* provider) {
    if (!deps) return UVHTTP_ERROR_INVALID_PARAM;
    deps->memory_provider = provider;
    return UVHTTP_OK;
}

uvhttp_result_t uvhttp_deps_set_network_provider(uvhttp_deps_t* deps, uvhttp_network_provider_t* provider) {
    if (!deps) return UVHTTP_ERROR_INVALID_PARAM;
    deps->network_provider = provider;
    return UVHTTP_OK;
}

uvhttp_result_t uvhttp_deps_set_file_provider(uvhttp_deps_t* deps, uvhttp_file_provider_t* provider) {
    if (!deps) return UVHTTP_ERROR_INVALID_PARAM;
    deps->file_provider = provider;
    return UVHTTP_OK;
}

uvhttp_loop_provider_t* uvhttp_deps_get_loop_provider(uvhttp_deps_t* deps) {
    return deps ? deps->loop_provider : NULL;
}

uvhttp_memory_provider_t* uvhttp_deps_get_memory_provider(uvhttp_deps_t* deps) {
    return deps ? deps->memory_provider : NULL;
}

uvhttp_network_provider_t* uvhttp_deps_get_network_provider(uvhttp_deps_t* deps) {
    return deps ? deps->network_provider : NULL;
}

uvhttp_file_provider_t* uvhttp_deps_get_file_provider(uvhttp_deps_t* deps) {
    return deps ? deps->file_provider : NULL;
}

uvhttp_deps_t* uvhttp_deps_create_default(void) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    if (!deps) return NULL;
    
    deps->loop_provider = uvhttp_default_loop_provider_new();
    deps->memory_provider = uvhttp_default_memory_provider_new();
    deps->network_provider = uvhttp_default_network_provider_new();
    deps->file_provider = uvhttp_default_file_provider_new();
    deps->owns_providers = true;
    
    return deps;
}

uvhttp_deps_t* uvhttp_deps_create_test(void) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    if (!deps) return NULL;
    
    deps->loop_provider = uvhttp_default_loop_provider_new();
    deps->memory_provider = uvhttp_test_memory_provider_new();
    deps->network_provider = uvhttp_test_network_provider_new();
    deps->file_provider = uvhttp_test_file_provider_new();
    deps->owns_providers = true;
    
    return deps;
}

void uvhttp_deps_free(uvhttp_deps_t* deps) {
    if (!deps) return;
    
    if (deps->cleanup) {
        deps->cleanup(deps);
    }
    
    uvhttp_free(deps);
}