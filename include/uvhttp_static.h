/* UVHTTP Static File Serving Module - Unified version with optional LRU cache
 */

#ifndef UVHTTP_STATIC_H
#define UVHTTP_STATIC_H

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"
#    include "uvhttp_middleware.h"

#    include <stddef.h>
#    include <time.h>

/* LRU cache conditional compilation support */
#    if UVHTTP_FEATURE_LRU_CACHE
#        include "uvhttp_lru_cache.h"
#    endif

/* Forward declarations */
typedef struct cache_manager cache_manager_t;
typedef struct cache_entry cache_entry_t;

#    ifdef __cplusplus
extern "C" {
#    endif

/* Static file configuration structure - CPU cache optimized layout */
typedef struct uvhttp_static_config {
    /* 8-byte aligned fields - hot path */
    size_t max_cache_size;      /* Maximum cache size (bytes) */
    size_t sendfile_chunk_size; /* sendfile chunk size (bytes) */
    size_t max_file_size;       /* Maximum file size (bytes) */

    /* 4-byte aligned fields - medium access frequency */
    int cache_ttl;                /* Cache TTL (seconds) */
    int max_cache_entries;        /* Maximum cache entries */
    int sendfile_timeout_ms;      /* sendfile timeout (milliseconds) */
    int sendfile_max_retry;       /* sendfile max retry count */
    int enable_directory_listing; /* Enable directory listing */
    int enable_etag;              /* Enable ETag */
    int enable_last_modified;     /* Enable Last-Modified */
    int enable_sendfile;          /* Enable sendfile zero-copy optimization */

    /* String fields - cold path */
    char root_directory[UVHTTP_MAX_FILE_PATH_SIZE];    /* Root directory path */
    char index_file[UVHTTP_MAX_PATH_SIZE];             /* Default index file */
    char custom_headers[UVHTTP_MAX_HEADER_VALUE_SIZE]; /* Custom response
                                                          headers */
} uvhttp_static_config_t;

/* Static file serving context */
typedef struct uvhttp_static_context {
    uvhttp_static_config_t config; /*  */
    cache_manager_t* cache;        /* LRUCachemanage */
} uvhttp_static_context_t;

/* MIME type mapping entry */
typedef struct uvhttp_mime_mapping {
    const char* extension; /* extend */
    const char* mime_type; /* MIMEclass */
} uvhttp_mime_mapping_t;

/**
 * createStatic file
 *
 * @param config Static file
 * @param context outputParameter, Returncreate
 * @return UVHTTP_OK Success, othervaluerepresentsError
 */
uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config,
                                    uvhttp_static_context_t** context);
/**
 * set sendfile Configuration parameter
 *
 * @param ctx Static file
 * @param timeout_ms Timeoutwhen(seconds), 0 representsUseDefault value
 * @param max_retry Retrytimes, 0 representsUseDefault value
 * @param chunk_size chunkedsize(bytes), 0 representsUseDefault value
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_static_set_sendfile_config(uvhttp_static_context_t* ctx,
                                                 int timeout_ms, int max_retry,
                                                 size_t chunk_size);

/**
 * set maximum file size limit
 *
 * @param ctx Static file context
 * @param max_file_size Maximum file size in bytes (0 means use default)
 * @return UVHTTP_OK Success, other values represent Failure
 */
uvhttp_error_t uvhttp_static_set_max_file_size(uvhttp_static_context_t* ctx,
                                               size_t max_file_size);

/**
 * releaseStatic file
 *
 * @param ctx Static file
 */
void uvhttp_static_free(uvhttp_static_context_t* ctx);

/**
 * handleStatic fileRequest
 *
 * @param ctx Static file
 * @param request HTTPRequest
 * @param response HTTPResponse
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                             void* request, void* response);

/**
 * Nginx optimization: Use sendfile zerosendStatic file(strategy)
 *
 * rootFile sizeAutomaticstrategy:
 * -  (< 4KB): Use( sendfile )
 * -  (4KB - 10MB): UseAsynchronous sendfile
 * -  (> 10MB): Usechunked sendfile
 *
 * @param file_path File path
 * @param response HTTPResponse
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_sendfile(const char* file_path, void* response);

/**
 * rootextendgetMIMEclass
 *
 * @param file_path File path
 * @param mime_type outputMIMEclassBuffer
 * @param buffer_size Buffersize
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path,
                                            char* mime_type,
                                            size_t buffer_size);

/**
 * Cache
 *
 * @param ctx Static file
 */
void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx);

/**
 * Cache: loadspecifiedtoCache
 *
 * @param ctx Static file
 * @param file_path File path(pairrootDirectory)
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,
                                            const char* file_path);

/**
 * Cache: loadDirectoryof
 *
 * @param ctx Static file
 * @param dir_path DirectoryPath(pairrootDirectory)
 * @param max_files (0represents)
 * @return quantity, -1representsFailure
 */
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path, int max_files);

/**
 * File pathwhether(preventPathtraverse)
 *
 * @param root_dir rootDirectory
 * @param file_path RequestFile path
 * @param resolved_path parsingPath
 * @param buffer_size Buffersize
 * @return 1, 0
 */
int uvhttp_static_resolve_safe_path(const char* root_dir, const char* file_path,
                                    char* resolved_path, size_t buffer_size);

/**
 * generateETagvalue
 *
 * @param file_path File path
 * @param last_modified lastmodifywhen
 * @param file_size File size
 * @param etag outputETagBuffer
 * @param buffer_size Buffersize
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_generate_etag(const char* file_path,
                                            time_t last_modified,
                                            size_t file_size, char* etag,
                                            size_t buffer_size);

/**
 * Request(If-None-Match, If-Modified-Since)
 *
 * @param request HTTPRequest
 * @param etag ETag
 * @param last_modified lastmodifywhen
 * @return 1needReturn304, 0needReturn
 */
int uvhttp_static_check_conditional_request(void* request, const char* etag,
                                            time_t last_modified);

/**
 * setStatic fileResponse
 *
 * @param response HTTPResponse
 * @param file_path File path
 * @param file_size File size
 * @param last_modified lastmodifywhen
 * @param etag ETagvalue
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_set_response_headers(void* response,
                                                   const char* file_path,
                                                   size_t file_size,
                                                   time_t last_modified,
                                                   const char* etag);

/**
 * getCachestatistics
 *
 * @param ctx Static file
 * @param total_memory_usage outputmemoryUse
 * @param entry_count outputentryquantity
 * @param hit_count outputtimes
 * @param miss_count outputtimes
 * @param eviction_count outputevicttimes
 */
void uvhttp_static_get_cache_stats(uvhttp_static_context_t* ctx,
                                   size_t* total_memory_usage, int* entry_count,
                                   int* hit_count, int* miss_count,
                                   int* eviction_count);

/**
 * getCacherate
 *
 * @param ctx Static file
 * @return rate(0.0-1.0)
 */
double uvhttp_static_get_cache_hit_rate(uvhttp_static_context_t* ctx);

/**
 * Cacheentry
 *
 * @param ctx Static file
 * @return entryquantity
 */
int uvhttp_static_cleanup_expired_cache(uvhttp_static_context_t* ctx);

/**
 * EnableCache(ifisDisable)
 *
 * @param ctx Static file
 * @param max_memory memoryUse
 * @param max_entries maximum entries
 * @param ttl CacheTTL(seconds)
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_result_t uvhttp_static_enable_cache(uvhttp_static_context_t* ctx,
                                           size_t max_memory, int max_entries,
                                           int ttl);

/**
 * DisableCache
 *
 * @param ctx Static file
 */
void uvhttp_static_disable_cache(uvhttp_static_context_t* ctx);

/* ========== Static File Middleware Interface ========== */

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_FEATURE_STATIC_FILES */

#endif /* UVHTTP_STATIC_H */