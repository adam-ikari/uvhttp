# Static File API Spec

## Overview

The Static File module serves static files from a root directory via HTTP. It provides configurable file serving with LRU cache integration, zero-copy sendfile (via libuv), MIME type detection, ETag/Last-Modified conditional requests, path traversal protection, and directory listing. The module is guarded by the `UVHTTP_FEATURE_STATIC_FILES` compile-time feature gate.

## Interfaces

### uvhttp_static_create
- **Signature**: `uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config, uvhttp_static_context_t** context)`
- **Purpose**: Create a static file serving context with an LRU cache
- **Preconditions**: `config` must be non-NULL with `root_directory` set to a valid path. `context` must be a non-NULL pointer to `uvhttp_static_context_t*`.
- **Postconditions**: On success, `*context` points to a valid context with an initialized LRU cache. On failure, `*context` is unchanged and no memory is allocated.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `config` or `context` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
  - `UVHTTP_ERROR_IO_ERROR`: cache creation failed
- **Thread safety**: Not thread-safe.
- **Feature gate**: `#if UVHTTP_FEATURE_STATIC_FILES`

### uvhttp_static_free
- **Signature**: `void uvhttp_static_free(uvhttp_static_context_t* ctx)`
- **Purpose**: Free a static file context and all associated resources
- **Preconditions**: `ctx` can be NULL (no-op). Must have been created by `uvhttp_static_create`.
- **Postconditions**: The LRU cache is freed and the context memory is released. The pointer is invalid after return.
- **Thread safety**: Not thread-safe.

### uvhttp_static_handle_request
- **Signature**: `uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx, void* request, void* response)`
- **Purpose**: Handle an HTTP request by serving a static file
- **Preconditions**: `ctx` must be valid. `request` and `response` must be valid `uvhttp_request_t`/`uvhttp_response_t` pointers.
- **Postconditions**: On success, the file is served with appropriate headers. The response is sent via `uvhttp_response_send`. On failure, an appropriate HTTP status is set on the response.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: any argument is NULL
  - `UVHTTP_ERROR_MALFORMED_REQUEST`: URL is NULL
  - `UVHTTP_ERROR_HEADER_TOO_LARGE`: URL path exceeds buffer
  - `UVHTTP_ERROR_NOT_FOUND`: file not found, returns 403 or 404
  - `UVHTTP_ERROR_FILE_TOO_LARGE`: file exceeds `max_file_size`, returns 413
  - `UVHTTP_ERROR_IO_ERROR`: file read failure, returns 500
- **Thread safety**: Not thread-safe. Must be called from the event loop thread.
- **Send strategy**: Files < 4KB use direct read; files 4KB-10MB use chunked async sendfile; files > 10MB use sendfile zero-copy; on sendfile failure, falls back to chunked transfer.

### uvhttp_static_get_mime_type
- **Signature**: `uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path, char* mime_type, size_t buffer_size)`
- **Purpose**: Get the MIME type for a file based on its extension
- **Preconditions**: `file_path` must be non-NULL. `mime_type` must be a non-NULL buffer of at least `buffer_size` bytes.
- **Postconditions**: `mime_type` is filled with the MIME type string. Returns `"application/octet-stream"` for unknown extensions.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `file_path` or `mime_type` is NULL, or `buffer_size` is 0
- **Thread safety**: Thread-safe. Uses a static hash table for O(1) lookup.
- **Performance**: Hash table lookup for common extensions (html, css, js, png, jpg) with fallback linear scan for hash collisions.

### uvhttp_static_is_safe_path
- **Signature** (internal): The module provides `uvhttp_static_resolve_safe_path(const char* root_dir, const char* file_path, char* resolved_path, size_t buffer_size)`
- **Purpose**: Resolve a file path and verify it is within the root directory (path traversal prevention)
- **Preconditions**: `root_dir`, `file_path`, and `resolved_path` must be non-NULL. `buffer_size` must be > 0.
- **Postconditions**: On success, `resolved_path` contains the canonicalized absolute path. Returns 1 if safe, 0 otherwise.
- **Error conditions**:
  - Returns 0 if any argument is NULL or buffer_size is 0
  - Returns 0 if the path fails URL validation (`uvhttp_validate_url_path`)
  - Returns 0 if the canonicalized path is not under the root directory
  - Returns 0 if the path does not exist (realpath fails)
- **Thread safety**: Not thread-safe.

### uvhttp_static_generate_etag
- **Signature**: `uvhttp_result_t uvhttp_static_generate_etag(const char* file_path, time_t last_modified, size_t file_size, char* etag, size_t buffer_size)`
- **Purpose**: Generate an ETag value for a file using `"<file_size>-<last_modified>"` format
- **Preconditions**: `file_path` and `etag` must be non-NULL. `buffer_size` must be > 0.
- **Postconditions**: `etag` is filled with a quoted string in the format `"<size>-<mtime>"`.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `file_path` or `etag` is NULL, or `buffer_size` is 0
- **Thread safety**: Thread-safe.

### uvhttp_static_prewarm_cache
- **Signature**: `uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx, const char* file_path)`
- **Purpose**: Preload a specific file into the LRU cache (relative to root directory)
- **Preconditions**: `ctx` must be valid with a cache. `file_path` must be non-NULL and relative to the root directory.
- **Postconditions**: The file is read from disk and stored in the LRU cache with MIME type, ETag, and last-modified time.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `ctx` or `file_path` is NULL, or path is too long
  - `UVHTTP_ERROR_NOT_FOUND`: file does not exist or is a directory
  - `UVHTTP_ERROR_INVALID_PARAM`: file exceeds `max_file_size`
  - `UVHTTP_ERROR_SERVER_INIT`: file read failure
  - Cache put failure is logged but does not return an error
- **Thread safety**: Not thread-safe.

### uvhttp_static_prewarm_directory
- **Signature**: `int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx, const char* dir_path, int max_files)`
- **Purpose**: Preload all files in a directory into the LRU cache
- **Preconditions**: `ctx` must be valid with a cache. `dir_path` must be non-NULL and relative to the root directory. If `max_files` > 0, at most `max_files` files are prewarmed.
- **Postconditions**: Returns the number of successfully prewarmed files. Returns -1 on failure.
- **Error conditions**:
  - Returns -1: `ctx` or `dir_path` is NULL, cache is not initialized, directory path is too long, or directory does not exist
- **Thread safety**: Not thread-safe.

### uvhttp_static_clear_cache
- **Signature**: `void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx)`
- **Purpose**: Clear all entries from the LRU cache
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: All cache entries are freed. Cache hit/miss statistics are preserved.
- **Thread safety**: Not thread-safe.

### uvhttp_static_get_cache_stats
- **Signature**: `void uvhttp_static_get_cache_stats(uvhttp_static_context_t* ctx, size_t* total_memory_usage, int* entry_count, int* hit_count, int* miss_count, int* eviction_count)`
- **Purpose**: Get cache statistics
- **Preconditions**: `ctx` must be valid. Output pointers can be NULL.
- **Postconditions**: Output parameters are filled with current cache statistics. If `ctx` or cache is NULL, all outputs are set to 0.
- **Thread safety**: Not thread-safe.

### uvhttp_static_get_cache_hit_rate
- **Signature**: `double uvhttp_static_get_cache_hit_rate(uvhttp_static_context_t* ctx)`
- **Purpose**: Get the cache hit rate as a percentage (0.0-100.0)
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Returns 0.0 if the cache is NULL or no requests have been made.
- **Thread safety**: Not thread-safe.

### uvhttp_static_cleanup_expired_cache
- **Signature**: `int uvhttp_static_cleanup_expired_cache(uvhttp_static_context_t* ctx)`
- **Purpose**: Remove all expired cache entries
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Returns the number of entries removed. Returns 0 if cache is NULL.
- **Thread safety**: Not thread-safe.

### uvhttp_static_set_sendfile_config
- **Signature**: `uvhttp_error_t uvhttp_static_set_sendfile_config(uvhttp_static_context_t* ctx, int timeout_ms, int max_retry, size_t chunk_size)`
- **Purpose**: Configure sendfile parameters (timeout, retry, chunk size)
- **Preconditions**: `ctx` must be valid. Values of 0 mean "use current/default".
- **Postconditions**: The configuration values are updated.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_static_sendfile
- **Signature**: `uvhttp_result_t uvhttp_static_sendfile(const char* file_path, void* response)`
- **Purpose**: Send a file using zero-copy sendfile (uses default config)
- **Preconditions**: `file_path` must be non-NULL and point to an existing file. `response` must be a valid `uvhttp_response_t*`.
- **Postconditions**: File is sent with automatic strategy selection based on file size.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: any argument is NULL
  - `UVHTTP_ERROR_NOT_FOUND`: file does not exist
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
  - `UVHTTP_ERROR_RESPONSE_SEND`: sendfile syscall failure
  - `UVHTTP_ERROR_SERVER_INIT`: failed to get client fd
- **Thread safety**: Not thread-safe.

### uvhttp_static_check_conditional_request
- **Signature**: `int uvhttp_static_check_conditional_request(void* request, const char* etag, time_t last_modified)`
- **Purpose**: Check If-None-Match and If-Modified-Since headers
- **Preconditions**: `request` must be non-NULL.
- **Postconditions**: Returns 1 if the client's cached version is still valid (return 304), 0 otherwise.
- **Thread safety**: Thread-safe for reads.

### uvhttp_static_set_response_headers
- **Signature**: `uvhttp_result_t uvhttp_static_set_response_headers(void* response, const char* file_path, size_t file_size, time_t last_modified, const char* etag)`
- **Purpose**: Set Content-Type, Content-Length, Last-Modified, ETag, and Cache-Control headers
- **Preconditions**: `response` and `file_path` must be non-NULL.
- **Postconditions**: Response headers are set for the file.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `response` or `file_path` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_static_enable_cache / uvhttp_static_disable_cache
- **Signature**: `uvhttp_result_t uvhttp_static_enable_cache(uvhttp_static_context_t* ctx, size_t max_memory, int max_entries, int ttl)` / `void uvhttp_static_disable_cache(uvhttp_static_context_t* ctx)`
- **Purpose**: Enable or disable the LRU cache at runtime
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Cache is enabled with the specified parameters or disabled (all entries freed).
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `ctx` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: cache allocation failure

### uvhttp_static_set_cache_config
- **Signature**: `uvhttp_error_t uvhttp_static_set_cache_config(uvhttp_static_context_t* ctx, size_t max_cache_size, int max_entries, int cache_ttl)`
- **Purpose**: Update cache configuration parameters at runtime
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Cache parameters are updated. Values of 0 mean "no change".
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

## Behavior Rules

1. **Root directory isolation**: All file paths are resolved relative to the configured root directory. Path traversal is prevented via `realpath` canonicalization and prefix comparison.

2. **Send strategy by file size**: Files smaller than 4KB use direct read I/O. Files between 4KB and 10MB use chunked async sendfile. Files larger than 10MB use sendfile zero-copy. On sendfile failure, the system falls back to chunked transfer.

3. **LRU cache integration**: The cache stores file content, MIME type, ETag, and last-modified time. Cache lookups use the canonicalized file path as key. Cache expiry is based on TTL (default 3600 seconds).

4. **Conditional request handling**: ETag (If-None-Match) is checked first. Last-Modified (If-Modified-Since) is checked second. If either indicates the cached version is valid, a 304 Not Modified response is returned.

5. **Directory listing**: When enabled, directory requests generate an auto-index HTML page with entries sorted by type (directories first) then by name. File names are HTML-escaped to prevent XSS.

6. **Index file fallback**: Directory requests without an index file fall back to directory listing (if enabled) or return 404.

7. **File size limit**: Files exceeding `max_file_size` return a 413 Payload Too Large response to prevent DoS.

8. **Double-free protection**: `uvhttp_static_free` handles NULL gracefully. Context memory is freed exactly once.

9. **sendfile retry**: On EINTR/EAGAIN, sendfile retries up to `sendfile_max_retry` times (default 2). Timeout is enforced via a timer (default 30 seconds).

10. **TCP_CORK optimization**: For large file sends via sendfile, TCP_CORK is enabled to coalesce packets and disabled on completion.

## Performance Requirements

- MIME type lookup: O(1) via hash table (with linear fallback for collisions)
- Safe path resolution: O(n) where n is path length
- ETag generation: O(1)
- Cache lookup: O(1) amortized via LRU hash table
- sendfile: O(1) per chunk; zero-copy kernel-space transfer
- Memory: ~256 bytes per context instance, plus per-file cache entries
- Cache eviction: batch eviction (default 10 entries) for amortized O(1) insertion

## Test Requirements

- Creation and destruction of static context (with and without cache)
- File serving with various file sizes (small, medium, large)
- MIME type detection for all supported extensions
- Unknown extension returns "application/octet-stream"
- Path traversal attack prevention (e.g., `../../etc/passwd`)
- Conditional request handling (If-None-Match, If-Modified-Since)
- 304 Not Modified response correctness
- Directory listing generation and HTML escaping
- Cache prewarm (single file and directory)
- Cache statistics and hit rate calculation
- Cache expiry and cleanup
- sendfile fallback on failure
- sendfile timeout and retry behavior
- File size limit enforcement (413 response)
- NULL parameter handling for all public functions
- Memory cleanup (no leaks on free)
- Feature gate: builds without UVHTTP_FEATURE_STATIC_FILES