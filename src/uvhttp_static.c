/* UVHTTP static file service module implementation - V2, integrated with LRU
 * cache */

#if UVHTTP_FEATURE_STATIC_FILES

#    define _XOPEN_SOURCE 600 /* Enable strptime, timegm */
#    define _DEFAULT_SOURCE   /* Enable strcasecmp */

#    include "uvhttp_static.h"

#    include "uvhttp_allocator.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_error_handler.h"
#    include "uvhttp_error_helpers.h"
#    include "uvhttp_logging.h"
#    include "uvhttp_lru_cache.h"
#    include "uvhttp_middleware.h"
#    include "uvhttp_platform.h"
#    include "uvhttp_request.h"
#    include "uvhttp_response.h"
#    include "uvhttp_utils.h"
#    include "uvhttp_validation.h"

#    include <dirent.h>
#    include <errno.h>
#    include <fcntl.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/stat.h>
#    include <time.h>

/* MIME type mapping table */
static const uvhttp_mime_mapping_t mime_types[] = {
    /* Text types */
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".md", "text/markdown"},
    {".csv", "text/csv"},

    /* Image types */
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".webp", "image/webp"},
    {".bmp", "image/bmp"},

    /* audio type */
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".ogg", "audio/ogg"},
    {".aac", "audio/aac"},

    /* video type */
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".avi", "video/x-msvideo"},

    /* font type */
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".eot", "application/vnd.ms-fontobject"},

    /* applicationtype */
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},

    /* defaulttype */
    {".", "application/octet-stream"},
    {NULL, NULL}};

/**
 * getfileextension
 */
static const char* get_file_extension(const char* file_path) {
    if (!file_path)
        return NULL;

    const char* last_dot = strrchr(file_path, '.');
    return last_dot ? last_dot : "";
}

/* forward declaration */
static uvhttp_result_t uvhttp_static_sendfile_with_config(
    const char* file_path, void* response,
    const uvhttp_static_config_t* config);

/**
 * HTML escape function - prevent XSS attack
 */
static void html_escape(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return;
    }

    size_t i = 0;
    size_t j = 0;

    while (src[i] != '\0' && j < dest_size - 1) {
        switch (src[i]) {
        case '<':
            if (j + 4 < dest_size) {
                dest[j++] = '&';
                dest[j++] = 'l';
                dest[j++] = 't';
                dest[j++] = ';';
            }
            break;
        case '>':
            if (j + 4 < dest_size) {
                dest[j++] = '&';
                dest[j++] = 'g';
                dest[j++] = 't';
                dest[j++] = ';';
            }
            break;
        case '&':
            if (j + 5 < dest_size) {
                dest[j++] = '&';
                dest[j++] = 'a';
                dest[j++] = 'm';
                dest[j++] = 'p';
                dest[j++] = ';';
            }
            break;
        case '"':
            if (j + 6 < dest_size) {
                dest[j++] = '&';
                dest[j++] = 'q';
                dest[j++] = 'u';
                dest[j++] = 'o';
                dest[j++] = 't';
                dest[j++] = ';';
            }
            break;
        case '\'':
            if (j + 6 < dest_size) {
                dest[j++] = '&';
                dest[j++] = '#';
                dest[j++] = '3';
                dest[j++] = '9';
                dest[j++] = ';';
            }
            break;
        default:
            dest[j++] = src[i];
            break;
        }
        i++;
    }

    dest[j] = '\0';
}

/**
 * get MIME type according to file extension
 */
uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path,
                                            char* mime_type,
                                            size_t buffer_size) {
    if (!file_path || !mime_type || buffer_size == 0)
        return UVHTTP_ERROR_INVALID_PARAM;

    const char* extension = get_file_extension(file_path);

    /* findMIMEtype */
    for (int i = 0; mime_types[i].extension; i++) {
        if (strcasecmp(extension, mime_types[i].extension) == 0) {
            if (uvhttp_safe_strncpy(mime_type, mime_types[i].mime_type,
                                    buffer_size) != 0) {
                UVHTTP_LOG_ERROR("Failed to copy MIME type: %s",
                                 mime_types[i].mime_type);
            }
            return UVHTTP_OK;
        }
    }

    /* defaultMIMEtype */
    if (uvhttp_safe_strncpy(mime_type, "application/octet-stream",
                            buffer_size) != 0) {
        UVHTTP_LOG_ERROR("Failed to copy default MIME type");
    }

    return 0;
}

/**
 * readfilecontent
 */
static char* read_file_content(const char* file_path, size_t* file_size) {
    if (!file_path || !file_size)
        return NULL;

    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return NULL;
    }

    /* getfilesize */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    *file_size = (size_t)size;

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    /* allocate memory and read file */
    char* content = uvhttp_alloc(*file_size);
    if (!content) {
        fclose(file);
        uvhttp_handle_memory_failure("file_content", NULL, NULL);
        return NULL;
    }

    size_t bytes_read = fread(content, 1, *file_size, file);
    fclose(file);

    if (bytes_read != *file_size) {
        uvhttp_free(content);
        return NULL;
    }

    return content;
}

/**
 * chunked file transfer context
 * - used for streaming large files in chunks
 * - avoids loading entire file into memory
 */
typedef struct {
    FILE* file;
    size_t file_size;
    size_t bytes_sent;
    size_t chunk_size;
    char* chunk_buffer;
    uvhttp_response_t* response;
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
    time_t last_modified;
    char safe_path[UVHTTP_MAX_PATH_SIZE];
} chunked_transfer_context_t;

/**
 * send file in chunks (for large files when sendfile is not available)
 * - avoids loading entire file into memory
 * - uses fixed-size chunks to balance memory and performance
 */
static int send_file_chunked(const char* file_path, size_t file_size,
                             time_t last_modified, uvhttp_response_t* response,
                             const char* etag, const char* safe_path) {
    if (!file_path || !response)
        return UVHTTP_ERROR_INVALID_PARAM;

    /* open file */
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    /* allocate chunk buffer */
    char* chunk_buffer = uvhttp_alloc(UVHTTP_FILE_CHUNK_SIZE);
    if (!chunk_buffer) {
        fclose(file);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* set response headers (only once) */
    uvhttp_static_set_response_headers(response, safe_path, file_size,
                                       last_modified, etag);
    uvhttp_response_set_status(response, 200);

    /* send headers first */
    char* response_data = NULL;
    size_t response_length = 0;
    uvhttp_error_t err =
        uvhttp_response_build_data(response, &response_data, &response_length);

    if (err != UVHTTP_OK) {
        uvhttp_free(chunk_buffer);
        fclose(file);
        return err;
    }

    /* send headers with NULL response to avoid connection restart */
    err = uvhttp_response_send_raw(response_data, response_length,
                                   response->client, NULL);
    uvhttp_free(response_data);

    if (err != UVHTTP_OK) {
        uvhttp_free(chunk_buffer);
        fclose(file);
        return err;
    }

    /* send file in chunks */
    size_t bytes_sent = 0;
    while (bytes_sent < file_size) {
        size_t bytes_to_read = UVHTTP_FILE_CHUNK_SIZE;
        size_t remaining = file_size - bytes_sent;

        if (bytes_to_read > remaining) {
            bytes_to_read = remaining;
        }

        size_t bytes_read = fread(chunk_buffer, 1, bytes_to_read, file);
        if (bytes_read != bytes_to_read) {
            uvhttp_free(chunk_buffer);
            fclose(file);
            return UVHTTP_ERROR_IO_ERROR;
        }

        /* send chunk with NULL response to avoid connection restart */
        err = uvhttp_response_send_raw(chunk_buffer, bytes_read,
                                       response->client, NULL);
        if (err != UVHTTP_OK) {
            uvhttp_free(chunk_buffer);
            fclose(file);
            return err;
        }

        bytes_sent += bytes_read;
    }

    /* cleanup */
    uvhttp_free(chunk_buffer);
    fclose(file);

    return UVHTTP_OK;
}

/**
 * getfileinfo
 */
static int get_file_info(const char* file_path, size_t* file_size,
                         time_t* last_modified) {
    if (!file_path)
        return -1;

    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        return -1; /* not a regular file */
    }

    if (file_size)
        *file_size = st.st_size;
    if (last_modified)
        *last_modified = st.st_mtime;

    return 0;
}

/**
 * calculate buffer size needed for directory list
 */
static size_t calculate_dir_listing_buffer_size(const char* dir_path,
                                                size_t* entry_count) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return 0;
    }

    size_t buffer_size = UVHTTP_DIR_LISTING_BUFFER_SIZE;
    *entry_count = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }

        buffer_size += strlen(entry->d_name) + UVHTTP_DIR_ENTRY_HTML_OVERHEAD;
        (*entry_count)++;
    }

    closedir(dir);
    return buffer_size;
}

/**
 * directoryentrystructure
 */
typedef struct {
    char name[256];
    size_t size;
    time_t mtime;
    int is_dir;
} dir_entry_t;

/**
 * collect directory entry info
 */
static dir_entry_t* collect_dir_entries(const char* dir_path,
                                        size_t* actual_count) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return NULL;
    }

    /* first calculate entry count */
    size_t entry_count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        entry_count++;
    }

    /* allocatememory */
    dir_entry_t* entries = uvhttp_alloc(entry_count * sizeof(dir_entry_t));
    if (!entries) {
        closedir(dir);
        return NULL;
    }

    /* collect entry info */
    rewinddir(dir);
    *actual_count = 0;

    while ((entry = readdir(dir)) != NULL && *actual_count < entry_count) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }

        dir_entry_t* dir_entry = &entries[*actual_count];
        /* use safe string copy, auto-truncate if file name too long */
        uvhttp_safe_strncpy(dir_entry->name, entry->d_name,
                            sizeof(dir_entry->name));

        /* getfileinfo */
        char full_path[UVHTTP_MAX_FILE_PATH_SIZE];
        int written = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path,
                               entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(full_path)) {
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) == 0) {
            dir_entry->size = st.st_size;
            dir_entry->mtime = st.st_mtime;
            dir_entry->is_dir = S_ISDIR(st.st_mode);
        } else {
            dir_entry->size = 0;
            dir_entry->mtime = 0;
            dir_entry->is_dir = 0;
        }

        (*actual_count)++;
    }

    closedir(dir);
    return entries;
}

/**
 * sort directory entry
 */
static void sort_dir_entries(dir_entry_t* entries, size_t count) {
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = i + 1; j < count; j++) {
            /* directory first */
            if (!entries[i].is_dir && entries[j].is_dir) {
                dir_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
            /* sort by name for same type */
            else if (entries[i].is_dir == entries[j].is_dir) {
                if (strcmp(entries[i].name, entries[j].name) > 0) {
                    dir_entry_t temp = entries[i];
                    entries[i] = entries[j];
                    entries[j] = temp;
                }
            }
        }
    }
}

/**
 * generate directory list HTML
 */
static char* generate_directory_listing(const char* dir_path,
                                        const char* request_path) {
    if (!dir_path || !request_path) {
        return NULL;
    }

    /* calculatebuffersize */
    size_t entry_count = 0;
    size_t buffer_size =
        calculate_dir_listing_buffer_size(dir_path, &entry_count);
    if (buffer_size == 0) {
        return NULL;
    }

    /* allocatebuffer */
    char* html = uvhttp_alloc(buffer_size);
    if (!html) {
        return NULL;
    }

    /* startgenerateHTML */
    size_t offset = 0;
    offset +=
        snprintf(html + offset, buffer_size - offset,
                 "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head>\n"
                 "<meta charset=\"UTF-8\">\n"
                 "<title>Directory listing for %s</title>\n"
                 "<style>\n"
                 "body { font-family: Arial, sans-serif; margin: 20px; }\n"
                 "h1 { color: #333; }\n"
                 "table { border-collapse: collapse; width: 100%%; }\n"
                 "th, td { text-align: left; padding: 8px; border-bottom: 1px "
                 "solid #ddd; }\n"
                 "th { background-color: #f2f2f2; }\n"
                 "a { text-decoration: none; color: #0066cc; }\n"
                 "a:hover { text-decoration: underline; }\n"
                 ".dir { font-weight: bold; }\n"
                 ".size { text-align: right; color: #666; }\n"
                 "</style>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Directory listing for %s</h1>\n"
                 "<table>\n"
                 "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n",
                 request_path, request_path);

    /* add parent directory link */
    if (strcmp(request_path, "/") != 0) {
        offset += snprintf(html + offset, buffer_size - offset,
                           "<tr><td><a href=\"../\">../</a></td><td "
                           "class=\"dir\">-</td><td>-</td></tr>\n");
    }

    /* collect directory entry */
    size_t actual_count = 0;
    dir_entry_t* entries = collect_dir_entries(dir_path, &actual_count);
    if (!entries) {
        uvhttp_free(html);
        return NULL;
    }

    /* sort entry */
    sort_dir_entries(entries, actual_count);

    /* generate HTML table rows */
    for (size_t i = 0; i < actual_count; i++) {
        dir_entry_t* dir_entry = &entries[i];

        /* format modification time */
        char time_str[64];
        if (dir_entry->mtime > 0) {
            struct tm* tm_info = localtime(&dir_entry->mtime);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            time_str[0] = '-';
            time_str[1] = '\0';
        }

        /* HTML escape file name */
        char escaped_name[UVHTTP_MAX_FILE_PATH_SIZE * 6];
        html_escape(escaped_name, dir_entry->name, sizeof(escaped_name));

        /* generate table row */
        if (dir_entry->is_dir) {
            offset += snprintf(
                html + offset, buffer_size - offset,
                "<tr><td><a href=\"%s/\" class=\"dir\">%s/</a></td><td "
                "class=\"dir\">-</td><td>%s</td></tr>\n",
                dir_entry->name, escaped_name, time_str);
        } else {
            offset += snprintf(html + offset, buffer_size - offset,
                               "<tr><td><a href=\"%s\">%s</a></td><td "
                               "class=\"size\">%zu</td><td>%s</td></tr>\n",
                               dir_entry->name, escaped_name, dir_entry->size,
                               time_str);
        }
    }

    /* completeHTML */
    snprintf(html + offset, buffer_size - offset,
             "</table>\n"
             "<p style=\"margin-top: 20px; color: #666; font-size: small;\">"
             "%zu entries total"
             "</p>\n"
             "</body>\n"
             "</html>",
             actual_count);

    /* clean */
    uvhttp_free(entries);

    UVHTTP_LOG_DEBUG("Generated directory listing for %s (%zu entries)",
                     request_path, actual_count);

    return html;
}

/**
 * generateETagvalue
 */
uvhttp_result_t uvhttp_static_generate_etag(const char* file_path,
                                            time_t last_modified,
                                            size_t file_size, char* etag,
                                            size_t buffer_size) {
    if (!file_path || !etag || buffer_size == 0)
        return UVHTTP_ERROR_INVALID_PARAM;

    /* simple ETag generate: file size - modification time */
    snprintf(etag, buffer_size, "\"%zu-%ld\"", file_size, (long)last_modified);

    return UVHTTP_OK;
}

/**
 * set static file related response headers
 */
uvhttp_result_t uvhttp_static_set_response_headers(void* response,
                                                   const char* file_path,
                                                   size_t file_size,
                                                   time_t last_modified,
                                                   const char* etag) {
    if (!response)
        return UVHTTP_ERROR_INVALID_PARAM;

    /* setContent-Type */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    if (uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type)) ==
        0) {
        uvhttp_response_set_header(response, "Content-Type", mime_type);
    }

    /* setContent-Length */
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%zu", file_size);
    uvhttp_response_set_header(response, "Content-Length", content_length);

    /* setLast-Modified */
    if (last_modified > 0) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT",
                 gmtime(&last_modified));
        uvhttp_response_set_header(response, "Last-Modified", time_str);
    }

    /* setETag */
    if (etag && *etag) {
        uvhttp_response_set_header(response, "ETag", etag);
    }

    /* setCache-Control */
    uvhttp_response_set_header(
        response, "Cache-Control",
        "public, max-age=" UVHTTP_STRINGIFY(UVHTTP_CACHE_DEFAULT_TTL));

    return UVHTTP_OK;
}

/**
 * checkconditionrequest(If-None-Match, If-Modified-Since)
 */
int uvhttp_static_check_conditional_request(void* request, const char* etag,
                                            time_t last_modified) {
    if (!request)
        return 0;

    /* checkIf-None-Match */
    const char* if_none_match =
        uvhttp_request_get_header(request, "If-None-Match");
    if (if_none_match && etag && strcmp(if_none_match, etag) == 0) {
        return 1; /* return304 */
    }

    /* checkIf-Modified-Since */
    const char* if_modified_since =
        uvhttp_request_get_header(request, "If-Modified-Since");
    if (if_modified_since && last_modified > 0) {
        struct tm tm = {0};
        if (strptime(if_modified_since, "%a, %d %b %Y %H:%M:%S GMT", &tm)) {
            time_t if_time = mktime(&tm);
            if (if_time >= last_modified) {
                return 1; /* return304 */
            }
        }
    }

    return 0; /* need to return complete content */
}

/**
 * create static file service context
 */
uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config,
                                    uvhttp_static_context_t** context) {
    if (!config || !context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_static_context_t* ctx =
        uvhttp_alloc(sizeof(uvhttp_static_context_t));
    if (!ctx) {
        uvhttp_handle_memory_failure("static_context", NULL, NULL);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(ctx, 0, sizeof(uvhttp_static_context_t));

    /* copyconfig */
    memcpy(&ctx->config, config, sizeof(uvhttp_static_config_t));

    /* createLRUcache */
    uvhttp_error_t result = uvhttp_lru_cache_create(
        config->max_cache_size,           /* maximum memory usage */
        UVHTTP_CACHE_DEFAULT_MAX_ENTRIES, /* maximum entry count */
        config->cache_ttl,                /* cacheTTL */
        &ctx->cache                       /* outputparameter */
    );

    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to create LRU cache: %s",
                         uvhttp_error_string(result));
        uvhttp_free(ctx);
        return result;
    }

    /* set max file size in cache */
    if (config->max_file_size > 0) {
        uvhttp_lru_cache_set_max_file_size(ctx->cache, config->max_file_size);
    } else {
        uvhttp_lru_cache_set_max_file_size(ctx->cache,
                                           UVHTTP_STATIC_MAX_FILE_SIZE);
    }

    if (!ctx->cache) {
        uvhttp_free(ctx);
        return UVHTTP_ERROR_IO_ERROR;
    }

    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to create LRU cache: %s",
                         uvhttp_error_string(result));
        uvhttp_free(ctx);
        return result;
    }

    if (!ctx->cache) {
        uvhttp_free(ctx);
        return UVHTTP_ERROR_IO_ERROR;
    }

    *context = ctx;
    return UVHTTP_OK;
}

/**
 * release static file service context
 */
void uvhttp_static_free(uvhttp_static_context_t* ctx) {
    if (!ctx)
        return;

    if (ctx->cache) {
        uvhttp_lru_cache_free(ctx->cache);
    }

    uvhttp_free(ctx);
}

/**
 * check if file path is safe (prevent path traversal attack)
 */
int uvhttp_static_resolve_safe_path(const char* root_dir, const char* file_path,
                                    char* resolved_path, size_t buffer_size) {
    if (!root_dir || !file_path || !resolved_path || buffer_size == 0) {
        return 0;
    }

    /* verifyinputpath */
    if (!uvhttp_validate_url_path(file_path)) {
        return 0;
    }

    /* build complete path */
    int root_len = strlen(root_dir);
    int path_len = strlen(file_path);

    /* ensure path won't overflow buffer */
    if (root_len + path_len + 2 >= (int)buffer_size) {
        return 0;
    }

    /* use safe string operation to copy root directory */
    if (uvhttp_safe_strcpy(resolved_path, buffer_size, root_dir) != 0) {
        return 0;
    }

    /* add path separator (if needed) */
    if (root_len > 0 && root_dir[root_len - 1] != '/') {
        size_t current_len = strlen(resolved_path);
        if (current_len + 1 < buffer_size) {
            resolved_path[current_len] = '/';
            resolved_path[current_len + 1] = '\0';
        } else {
            return 0;
        }
    }

    /* processfilepath */
    const char* path_to_add = (file_path[0] == '/') ? file_path + 1 : file_path;
    size_t current_len = strlen(resolved_path);
    size_t add_len = strlen(path_to_add);

    if (current_len + add_len < buffer_size) {
        memcpy(resolved_path + current_len, path_to_add,
               add_len + 1);  // +1 for null terminator
    } else {
        return 0;
    }

    /* use realpath for path canonicalization, prevent path traversal attack */
    char realpath_buf[PATH_MAX];
    char* resolved = realpath(resolved_path, realpath_buf);

    if (!resolved) {
        /* path doesn't exist or is invalid */
        return 0;
    }

    /* also convert root directory to absolute path for comparison */
    char root_realpath_buf[PATH_MAX];
    char* root_resolved = realpath(root_dir, root_realpath_buf);
    if (!root_resolved) {
        /* root directory doesn't exist or is invalid */
        return 0;
    }

    /* ensure canonicalized path is within root directory */
    size_t root_dir_len = strlen(root_resolved);
    if (strncmp(resolved, root_resolved, root_dir_len) != 0) {
        /* path is not within root directory */
        return 0;
    }

    /* ensure path is under root directory (not root directory itself or parent
     * directory) */
    if (strlen(resolved) > root_dir_len && resolved[root_dir_len] != '/') {
        return 0;
    }

    /* copy canonicalized path back to output buffer */
    if (strlen(resolved) >= buffer_size) {
        return 0;
    }
    /* use safe string copy function */
    if (uvhttp_safe_strcpy(resolved_path, buffer_size, resolved) != 0) {
        return 0;
    }

    return 1;
}

/**
 * main function to process static file request
 */
uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                             void* request, void* response) {
    if (!ctx || !request || !response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    const char* url = uvhttp_request_get_url(request);
    if (!url) {
        uvhttp_response_set_status(response, 400);
        return UVHTTP_ERROR_MALFORMED_REQUEST;
    }

    /* parse URL path, remove query parameters */
    char clean_path[UVHTTP_MAX_PATH_SIZE];
    const char* query_start = strchr(url, '?');
    size_t path_len = query_start ? (size_t)(query_start - url) : strlen(url);

    if (path_len >= sizeof(clean_path)) {
        uvhttp_response_set_status(response, 414); /* URI Too Long */
        return UVHTTP_ERROR_HEADER_TOO_LARGE;
    }

    /* use safe string copy */
    memcpy(clean_path, url, path_len);
    clean_path[path_len] = '\0';

    /* process root path */
    if (strcmp(clean_path, "/") == 0) {
        if (uvhttp_safe_strncpy(clean_path, ctx->config.index_file,
                                sizeof(clean_path)) != 0) {
            /* index_file too long, use default value */
            strncpy(clean_path, "index.html", sizeof(clean_path) - 1);
            clean_path[sizeof(clean_path) - 1] = '\0';
        }
    }

    /* build safe file path */
    char safe_path[UVHTTP_MAX_FILE_PATH_SIZE];
    if (!uvhttp_static_resolve_safe_path(ctx->config.root_directory, clean_path,
                                         safe_path, sizeof(safe_path))) {
        uvhttp_response_set_status(response, 403); /* Forbidden */
        return UVHTTP_ERROR_NOT_FOUND;
    }

    /* checkcache */
    cache_entry_t* cache_entry = uvhttp_lru_cache_find(ctx->cache, safe_path);

    if (cache_entry) {
        /* send response from cache */
        if (uvhttp_static_check_conditional_request(
                request, cache_entry->etag, cache_entry->last_modified)) {
            uvhttp_response_set_status(response, 304); /* Not Modified */
        } else {
            uvhttp_static_set_response_headers(
                response, cache_entry->file_path, cache_entry->content_length,
                cache_entry->last_modified, cache_entry->etag);
            uvhttp_response_set_body(response, cache_entry->content,
                                     cache_entry->content_length);
            uvhttp_response_set_status(response, 200);
        }
        uvhttp_response_send(response);
        return UVHTTP_OK;
    }

    /* cache miss, read file */
    size_t file_size;
    time_t last_modified;

    /* getfileinfo */
    if (get_file_info(safe_path, &file_size, &last_modified) != 0) {
        /* check if it's a directory */
        struct stat st;
        if (stat(safe_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (ctx->config.enable_directory_listing) {
                /* generate directory list */
                char* dir_html =
                    generate_directory_listing(safe_path, clean_path);
                if (dir_html) {
                    uvhttp_response_set_status(response, 200);
                    uvhttp_response_set_header(response, "Content-Type",
                                               "text/html");
                    uvhttp_response_set_body(response, dir_html,
                                             strlen(dir_html));
                    uvhttp_free(dir_html);
                    return UVHTTP_OK;
                }
            }
            /* attemptaddindexfile */
            char index_path[UVHTTP_MAX_FILE_PATH_SIZE];
            int result = snprintf(index_path, sizeof(index_path), "%s/%s",
                                  safe_path, ctx->config.index_file);
            if (result >= (int)sizeof(index_path)) {
                uvhttp_response_set_status(response, 414); /* URI Too Long */
                return UVHTTP_ERROR_HEADER_TOO_LARGE;
            }

            if (get_file_info(index_path, &file_size, &last_modified) == 0) {
                /* use safe string copy, auto-truncate if path too long */
                uvhttp_safe_strncpy(safe_path, index_path, sizeof(safe_path));
            } else {
                uvhttp_response_set_status(response, 404); /* Not Found */
                return UVHTTP_ERROR_NOT_FOUND;
            }
        } else {
            uvhttp_response_set_status(response, 404); /* Not Found */
            return UVHTTP_ERROR_NOT_FOUND;
        }
    }

    /* check file size limit to prevent DoS attacks */
    if (file_size > ctx->config.max_file_size) {
        UVHTTP_LOG_WARN("File too large: %s (size: %zu, limit: %zu)", safe_path,
                        file_size, ctx->config.max_file_size);
        uvhttp_response_set_status(response, 413); /* Payload Too Large */
        return UVHTTP_ERROR_FILE_TOO_LARGE;
    }

    /* for medium and large files (> 64KB), use sendfile zero-copy optimization
     * - performance optimization */
    if (file_size > UVHTTP_SENDFILE_MIN_FILE_SIZE) {
        /* getMIMEtype */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(safe_path, mime_type, sizeof(mime_type));

        /* generateETag */
        char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_generate_etag(safe_path, last_modified, file_size, etag,
                                    sizeof(etag));

        /* checkconditionrequest */
        if (uvhttp_static_check_conditional_request(request, etag,
                                                    last_modified)) {
            uvhttp_response_set_status(response, 304); /* Not Modified */
            return 0;
        }

        /* use sendfile to send (pass config) */
        uvhttp_result_t sendfile_result = uvhttp_static_sendfile_with_config(
            safe_path, response, &ctx->config);
        if (sendfile_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("sendfile failed: %s",
                             uvhttp_error_string(sendfile_result));
            /* fallback to chunked transfer for large files */
            /* chunked transfer avoids loading entire file into memory */
            int chunked_result = send_file_chunked(
                safe_path, file_size, last_modified, response, etag, safe_path);
            return chunked_result;
        } else {
            /* sendfilesuccess, return */
            return UVHTTP_OK;
        }
    }

    /* read file content (small file or fallback when sendfile fails) */
    char* file_content = read_file_content(safe_path, &file_size);
    if (!file_content) {
        uvhttp_response_set_status(response, 500); /* Internal Server Error */
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* getMIMEtype */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_get_mime_type(safe_path, mime_type, sizeof(mime_type));

    /* generateETag */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_generate_etag(safe_path, last_modified, file_size, etag,
                                sizeof(etag));

    /* checkconditionrequest */
    if (uvhttp_static_check_conditional_request(request, etag, last_modified)) {
        uvhttp_free(file_content);
        uvhttp_response_set_status(response, 304); /* Not Modified */
        return UVHTTP_OK;
    }

    /* add to cache */
    if (uvhttp_lru_cache_put(ctx->cache, safe_path, file_content, file_size,
                             mime_type, last_modified, etag) != 0) {
        /* cache add failure, but still need to return content */
        uvhttp_log_safe_error(0, "static_cache", "Failed to cache file");
    }

    /* sendresponse */
    uvhttp_static_set_response_headers(response, safe_path, file_size,
                                       last_modified, etag);
    uvhttp_response_set_body(response, file_content, file_size);
    uvhttp_response_set_status(response, 200);
    uvhttp_response_send(response);

    /* note: file content memory is now managed by cache, don't release here */

    return 0;
}

/**
 * cleanfilecache
 */
void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache)
        return;

    uvhttp_lru_cache_clear(ctx->cache);
}

/**
 * getcachestatisticsinfo
 */
void uvhttp_static_get_cache_stats(uvhttp_static_context_t* ctx,
                                   size_t* total_memory_usage, int* entry_count,
                                   int* hit_count, int* miss_count,
                                   int* eviction_count) {
    if (!ctx || !ctx->cache) {
        if (total_memory_usage)
            *total_memory_usage = 0;
        if (entry_count)
            *entry_count = 0;
        if (hit_count)
            *hit_count = 0;
        if (miss_count)
            *miss_count = 0;
        if (eviction_count)
            *eviction_count = 0;
        return;
    }

    uvhttp_lru_cache_get_stats(ctx->cache, total_memory_usage, entry_count,
                               hit_count, miss_count, eviction_count);
}

/**
 * get cache hit rate
 */
double uvhttp_static_get_cache_hit_rate(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache) {
        return 0.0;
    }

    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;

    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory_usage, &entry_count,
                               &hit_count, &miss_count, &eviction_count);

    if (hit_count + miss_count == 0) {
        return 0.0;
    }

    return (double)hit_count / (hit_count + miss_count) * 100.0;
}

/**
 * clean expired cache entry
 */
int uvhttp_static_cleanup_expired_cache(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache) {
        return 0;
    }

    return uvhttp_lru_cache_cleanup_expired(ctx->cache);
}

/* ========== static file middleware implementation ========== */

/**

 * cache prewarm: preload specified file to cache

 */

uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,

                                            const char* file_path) {

    if (!ctx || !file_path) {

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* build complete file path */

    char full_path[UVHTTP_MAX_FILE_PATH_SIZE];

    int result = snprintf(full_path, sizeof(full_path), "%s/%s",

                          ctx->config.root_directory, file_path);

    if (result >= (int)sizeof(full_path)) {

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check if file exists */
    struct stat st;
    if (stat(full_path, &st) != 0 || S_ISDIR(st.st_mode)) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    /* getfileinfo */
    size_t file_size = (size_t)st.st_size;
    time_t last_modified = st.st_mtime;

    /* checkfilesizelimit */
    if (file_size > ctx->config.max_file_size) {
        UVHTTP_LOG_WARN("File too large for prewarming: %s (size: %zu)",
                        file_path, file_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* readfilecontent */
    char* file_content = read_file_content(full_path, &file_size);
    if (!file_content) {
        UVHTTP_LOG_ERROR("Failed to read file for prewarming: %s", file_path);
        return UVHTTP_ERROR_SERVER_INIT;
    }

    /* getMIMEtype */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_get_mime_type(full_path, mime_type, sizeof(mime_type));

    /* generateETag */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_generate_etag(full_path, last_modified, file_size, etag,
                                sizeof(etag));

    /* add to cache */
    uvhttp_error_t cache_result =
        uvhttp_lru_cache_put(ctx->cache, full_path, file_content, file_size,
                             mime_type, last_modified, etag);
    if (cache_result != UVHTTP_OK) {
        UVHTTP_LOG_WARN("Failed to cache file for prewarming: %s", file_path);
        uvhttp_free(file_content);
        return cache_result;
    }

    UVHTTP_LOG_INFO("Prewarmed cache: %s (size: %zu)", file_path, file_size);
    return UVHTTP_OK;
}

/**
 * cache prewarm: preload all files in directory
 */
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path, int max_files) {
    if (!ctx || !dir_path) {
        UVHTTP_LOG_ERROR("Invalid parameters for directory prewarming");
        return -1;
    }

    if (!ctx->cache) {
        UVHTTP_LOG_ERROR("Cache not initialized for prewarming");
        return -1;
    }

    /* build complete directory path */
    char full_dir_path[UVHTTP_MAX_FILE_PATH_SIZE];
    int result = snprintf(full_dir_path, sizeof(full_dir_path), "%s/%s",
                          ctx->config.root_directory, dir_path);
    if (result >= (int)sizeof(full_dir_path)) {
        UVHTTP_LOG_ERROR("Directory path too long: %s", dir_path);
        return -1;
    }

    /* check if directory exists */
    struct stat st;
    if (stat(full_dir_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        UVHTTP_LOG_ERROR("Directory not found: %s", dir_path);
        return -1;
    }

    /* opendirectory */
    DIR* dir = opendir(full_dir_path);
    if (!dir) {
        UVHTTP_LOG_ERROR("Failed to open directory: %s (errno: %d)", dir_path,
                         errno);
        return -1;
    }

    int prewarmed_count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        /* skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* skipdirectory */
        if (entry->d_type == DT_DIR) {
            continue;
        }

        /* checkfilecountlimit */
        if (max_files > 0 && prewarmed_count >= max_files) {
            UVHTTP_LOG_INFO("Reached max files limit for prewarming: %d",
                            max_files);
            break;
        }

        /* build relative path */
        char relative_path[UVHTTP_MAX_FILE_PATH_SIZE];
        result = snprintf(relative_path, sizeof(relative_path), "%s/%s",
                          dir_path, entry->d_name);
        if (result < 0 || result >= (int)sizeof(relative_path)) {
            UVHTTP_LOG_WARN("File path too long, skipping: %s/%s", dir_path,
                            entry->d_name);
            continue;
        }

        /* prewarmfile */
        if (uvhttp_static_prewarm_cache(ctx, relative_path) == UVHTTP_OK) {
            prewarmed_count++;
        } else {
            UVHTTP_LOG_WARN("Failed to prewarm file: %s", relative_path);
        }
    }

    closedir(dir);

    UVHTTP_LOG_INFO("Prewarmed directory: %s (%d files)", dir_path,
                    prewarmed_count);
    return prewarmed_count;
}

/* ============ zero-copy optimization: sendfile implementation ============ */

/* sendfile contextstructure */
typedef struct {
    uv_fs_t open_req;
    uv_fs_t sendfile_req;
    uv_fs_t close_req;
    uv_file in_fd;
    uvhttp_response_t* response;
    char* file_path;
    size_t file_size;
    size_t bytes_sent;
    int64_t offset;
    int completed;
    uv_os_fd_t out_fd;        /* output file descriptor */
    uint64_t start_time;      /* start time (for timeout detection) */
    int retry_count;          /* retry count */
    int timeout_ms;           /* timeout time (milliseconds) */
    int max_retry;            /* maximum retry count */
    size_t chunk_size;        /* chunk size */
    int cork_enabled;         /* TCP_CORK enabled or not */
    uv_timer_t timeout_timer; /* timeout timer */
} sendfile_context_t;

/* sendfile default config (macro definition) - performance optimization */
#    define SENDFILE_DEFAULT_TIMEOUT_MS UVHTTP_SENDFILE_DEFAULT_TIMEOUT_MS
#    define SENDFILE_DEFAULT_MAX_RETRY UVHTTP_SENDFILE_DEFAULT_MAX_RETRY
#    define SENDFILE_DEFAULT_CHUNK_SIZE UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE
#    define SENDFILE_MIN_FILE_SIZE UVHTTP_SENDFILE_MIN_FILE_SIZE

/**
 * initialize sendfile context's config parameters
 *
 * @param ctx sendfile context
 * @param file_size filesize
 * @param config config parameter (can be NULL)
 */
static void init_sendfile_config(sendfile_context_t* ctx, size_t file_size,
                                 const uvhttp_static_config_t* config) {
    /* read sendfile parameters from config */
    if (config && config->sendfile_timeout_ms > 0) {
        ctx->timeout_ms = config->sendfile_timeout_ms;
    } else {
        ctx->timeout_ms = SENDFILE_DEFAULT_TIMEOUT_MS;
    }

    if (config && config->sendfile_max_retry > 0) {
        ctx->max_retry = config->sendfile_max_retry;
    } else {
        ctx->max_retry = SENDFILE_DEFAULT_MAX_RETRY;
    }

    if (config && config->sendfile_chunk_size > 0) {
        ctx->chunk_size = config->sendfile_chunk_size;
    } else {
        /* performance optimization: dynamically adjust chunk size based on file
         * size */
        if (file_size < UVHTTP_FILE_SIZE_SMALL) {
            /* small file (< 1MB): use smaller chunk, reduce memory usage */
            ctx->chunk_size = UVHTTP_CHUNK_SIZE_SMALL;
        } else if (file_size < UVHTTP_FILE_SIZE_MEDIUM) {
            /* medium file (1-10MB): use default chunk */
            ctx->chunk_size = SENDFILE_DEFAULT_CHUNK_SIZE;
        } else {
            /* large file (> 10MB): use larger chunk, reduce system call count
             */
            ctx->chunk_size = UVHTTP_CHUNK_SIZE_LARGE;
        }
    }
}

/* sendfile callbackfunction */
/* fileclosecallback */
static void on_file_close(uv_fs_t* req);

/* timeoutcallbackfunction */
static void on_sendfile_timeout(uv_timer_t* timer) {
    sendfile_context_t* ctx = (sendfile_context_t*)timer->data;

    if (!ctx || ctx->completed) {
        return;
    }

    UVHTTP_LOG_ERROR("sendfile timeout: %s (sent %zu/%zu bytes, elapsed %dms)",
                     ctx->file_path, ctx->bytes_sent, ctx->file_size,
                     ctx->timeout_ms);

    /* mark as complete, prevent duplicate processing */
    ctx->completed = 1;

    /* stop timer */
    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, NULL);

    /* close file and clean resources */
    uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)ctx->response->client);
    uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
}

/* sendfile callbackfunction */
/* fileclosecallback */
static void on_file_close(uv_fs_t* req) {
    sendfile_context_t* ctx = (sendfile_context_t*)req->data;

    if (req->result < 0) {
        UVHTTP_LOG_ERROR("Failed to close file: %s", uv_strerror(req->result));
    }

    uv_fs_req_cleanup(req);

    /* releasecontextmemory */
    if (ctx) {
        if (ctx->file_path) {
            uvhttp_free(ctx->file_path);
            ctx->file_path = NULL;
        }
        uvhttp_free(ctx);
    }
}

/* sendfile callbackfunction */
static void on_sendfile_complete(uv_fs_t* req) {
    sendfile_context_t* ctx = (sendfile_context_t*)req->data;

    if (!ctx || ctx->completed) {
        return;
    }

    /* getevent loop */
    uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)ctx->response->client);

    /* stop timeout timer (if still running) */
    if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
        uv_timer_stop(&ctx->timeout_timer);
        uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
    }

    /* check if send complete or error */
    if (req->result < 0) {
        UVHTTP_LOG_ERROR("sendfile failed: %s", uv_strerror(req->result));

        /* check if can retry */
        if (ctx->retry_count < ctx->max_retry &&
            (req->result == UV_EINTR || req->result == UV_EAGAIN)) {
            ctx->retry_count++;
            UVHTTP_LOG_INFO("Retrying sendfile: %s (attempt %d/%d)",
                            ctx->file_path, ctx->retry_count, ctx->max_retry);

            /* retrysend */
            size_t remaining = ctx->file_size - ctx->offset;
            size_t chunk_size =
                (remaining > ctx->chunk_size) ? ctx->chunk_size : remaining;

            uv_fs_req_cleanup(req);
            uv_fs_sendfile(loop, &ctx->sendfile_req, ctx->out_fd, ctx->in_fd,
                           ctx->offset, chunk_size, on_sendfile_complete);
            return;
        }

        /* send failure, close file and clean */
        uv_fs_req_cleanup(req);
        uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
        return;
    }

    ctx->bytes_sent += req->result;
    ctx->offset += req->result;

    /* check if send complete */
    if (ctx->offset >= (int64_t)ctx->file_size) {
        /* stop timeout timer */
        if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
        }

        /* closefile */
        uv_fs_req_cleanup(req);
        uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);

        /* mark complete */
        ctx->completed = 1;

        /* performance optimization: disable TCP_CORK to send buffered data */
        if (ctx->cork_enabled) {
            int cork = 0;
            setsockopt(ctx->out_fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
            ctx->cork_enabled = 0;
        }

        UVHTTP_LOG_INFO("sendfile completed: %s (%zu bytes)", ctx->file_path,
                        ctx->bytes_sent);
    } else {
        /* continue sending remaining data */
        size_t remaining = ctx->file_size - ctx->offset;
        size_t chunk_size =
            (remaining > ctx->chunk_size) ? ctx->chunk_size : remaining;

        uv_fs_req_cleanup(req);
        uv_fs_sendfile(loop, &ctx->sendfile_req, ctx->out_fd, ctx->in_fd,
                       ctx->offset, chunk_size, on_sendfile_complete);

        /* restart timeout timer */
        if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
            uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout,
                           ctx->timeout_ms, 0);
        }
    }
}

/**
 * set sendfile configparameter
 */
uvhttp_error_t uvhttp_static_set_sendfile_config(uvhttp_static_context_t* ctx,
                                                 int timeout_ms, int max_retry,
                                                 size_t chunk_size) {
    if (!ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* set config value (0 means use default value) */
    if (timeout_ms > 0) {
        ctx->config.sendfile_timeout_ms = timeout_ms;
    }
    if (max_retry > 0) {
        ctx->config.sendfile_max_retry = max_retry;
    }
    if (chunk_size > 0) {
        ctx->config.sendfile_chunk_size = chunk_size;
    }

    return UVHTTP_OK;
}

/**
 * set maximum file size limit
 */
uvhttp_error_t uvhttp_static_set_max_file_size(uvhttp_static_context_t* ctx,
                                               size_t max_file_size) {
    if (!ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* set config value (0 means use default value from
     * UVHTTP_STATIC_MAX_FILE_SIZE) */
    if (max_file_size > 0) {
        ctx->config.max_file_size = max_file_size;
    } else {
        ctx->config.max_file_size = UVHTTP_STATIC_MAX_FILE_SIZE;
    }

    return UVHTTP_OK;
}

/* internal function: sendfile with config */
static uvhttp_result_t uvhttp_static_sendfile_with_config(
    const char* file_path, void* response,
    const uvhttp_static_config_t* config) {
    uvhttp_response_t* resp = (uvhttp_response_t*)response;

    /* getfilesize */
    struct stat st;
    if (stat(file_path, &st) != 0) {
        UVHTTP_LOG_ERROR("Failed to stat file: %s", file_path);
        return UVHTTP_ERROR_NOT_FOUND;
    }

    size_t file_size = (size_t)st.st_size;

    /* strategy selection */
    if (file_size < UVHTTP_STATIC_SMALL_FILE_THRESHOLD) {
        /* small file: use optimized system call (open + read + close), avoid
         * stdio overhead */
        UVHTTP_LOG_DEBUG(
            "Small file detected, using optimized I/O: %s (%zu bytes)",
            file_path, file_size);

        /* use open() instead of fopen(), reduce system call overhead */
        int fd = open(file_path, O_RDONLY);
        if (fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file: %s", file_path);
            return UVHTTP_ERROR_NOT_FOUND;
        }

        /* readfilecontent */
        char* buffer = (char*)uvhttp_alloc(file_size + 1);
        if (!buffer) {
            close(fd);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }

        ssize_t bytes_read = read(fd, buffer, (size_t)file_size);
        close(fd);

        if (bytes_read < 0) {
            UVHTTP_LOG_ERROR("Failed to read file: %s", file_path);
            uvhttp_free(buffer);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }

        /* setresponse */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type));

        uvhttp_response_set_status(resp, 200);
        uvhttp_response_set_header(resp, "Content-Type", mime_type);
        uvhttp_response_set_header(resp, "Content-Length", "");
        uvhttp_response_set_body(resp, buffer, (size_t)bytes_read);

        uvhttp_free(buffer);
        return UVHTTP_OK;
    } else if (file_size <= UVHTTP_FILE_SIZE_MEDIUM) {
        /* medium file: use chunked async sendfile (same as large file) */
        UVHTTP_LOG_DEBUG("Medium file detected, using chunked async sendfile: "
                         "%s (%zu bytes)",
                         file_path, file_size);

        /* getevent loop */
        uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)resp->client);

        /* create sendfile context */
        sendfile_context_t* ctx =
            (sendfile_context_t*)uvhttp_alloc(sizeof(sendfile_context_t));
        if (!ctx) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }

        memset(ctx, 0, sizeof(sendfile_context_t));
        ctx->response = resp;
        ctx->file_size = file_size;
        ctx->offset = 0;
        ctx->bytes_sent = 0;
        ctx->completed = 0;
        ctx->start_time = uv_now(loop);
        ctx->retry_count = 0;
        ctx->sendfile_req.data = ctx; /* setcallbackdata */
        ctx->cork_enabled = 0;        /* initialize as disabled */

        /* initializeconfigparameter */
        init_sendfile_config(ctx, file_size, config);

        /* allocatefilepathmemory */
        size_t path_len = strlen(file_path);
        ctx->file_path = (char*)uvhttp_alloc(path_len + 1);
        if (!ctx->file_path) {
            uvhttp_free(ctx);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(ctx->file_path, file_path, path_len);
        ctx->file_path[path_len] = '\0';

        /* get output file descriptor */
        int fd_result = uv_fileno((uv_handle_t*)resp->client, &ctx->out_fd);
        if (fd_result < 0) {
            UVHTTP_LOG_ERROR("Failed to get client fd: %s",
                             uv_strerror(fd_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }

        /* openinputfile */
        ctx->in_fd = open(file_path, O_RDONLY);
        if (ctx->in_fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file for sendfile: %s", file_path);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_NOT_FOUND;
        }

        /* getMIMEtype */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type));

        char content_length[64];
        snprintf(content_length, sizeof(content_length), "%zu", file_size);

        uvhttp_response_set_status(resp, 200);
        uvhttp_response_set_header(resp, "Content-Type", mime_type);
        uvhttp_response_set_header(resp, "Content-Length", content_length);

        /* build response header data */
        char* header_data = NULL;
        size_t header_length = 0;
        uvhttp_error_t build_result =
            uvhttp_response_build_data(resp, &header_data, &header_length);
        if (build_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("Failed to build response headers: %s",
                             uvhttp_error_string(build_result));
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            return build_result;
        }

        /* send response headers (using send_raw, don't mark response as
         * complete) */
        uvhttp_error_t send_result = uvhttp_response_send_raw(
            header_data, header_length, resp->client, resp);
        uvhttp_free(header_data); /* release built response header data */

        if (send_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("Failed to send response headers: %s",
                             uvhttp_error_string(send_result));
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            return send_result;
        }

        /* performance optimization: enable TCP_CORK to optimize large file
         * transmission */
        int cork = 1;
        setsockopt(ctx->out_fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
        ctx->cork_enabled = 1;

        /* initialize timeout timer */
        int timer_result = uv_timer_init(loop, &ctx->timeout_timer);
        if (timer_result != 0) {
            UVHTTP_LOG_ERROR("Failed to init timeout timer: %s",
                             uv_strerror(timer_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        ctx->timeout_timer.data = ctx;

        /* start chunked sendfile (send config's chunk size each time) */
        size_t chunk_size = ctx->chunk_size;

        uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout,
                       ctx->timeout_ms, 0);

        int sendfile_result =
            uv_fs_sendfile(loop, &ctx->sendfile_req, ctx->out_fd, ctx->in_fd,
                           ctx->offset, chunk_size, on_sendfile_complete);

        /* check if sendfile failed synchronously or no data */
        if (sendfile_result < 0) {
            UVHTTP_LOG_ERROR("Failed to start sendfile: %s",
                             uv_strerror(sendfile_result));
            /* clean resources */
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }

        return UVHTTP_OK;
    } else {
        /* large file: use sendfile zero-copy optimization */
        UVHTTP_LOG_DEBUG("Large file detected, using sendfile: %s (%zu bytes)",
                         file_path, file_size);

        /* getevent loop */
        uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)resp->client);

        /* create sendfile context */
        sendfile_context_t* ctx =
            (sendfile_context_t*)uvhttp_alloc(sizeof(sendfile_context_t));
        if (!ctx) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }

        memset(ctx, 0, sizeof(sendfile_context_t));
        ctx->response = resp;
        ctx->file_size = file_size;
        ctx->offset = 0;
        ctx->bytes_sent = 0;
        ctx->completed = 0;
        ctx->start_time = uv_now(loop);
        ctx->retry_count = 0;
        ctx->cork_enabled = 0;        /* initialize as disabled */
        ctx->sendfile_req.data = ctx; /* setcallbackdata */

        /* initializeconfigparameter */
        init_sendfile_config(ctx, file_size, config);

        /* allocatefilepathmemory */
        size_t path_len = strlen(file_path);
        ctx->file_path = (char*)uvhttp_alloc(path_len + 1);
        if (!ctx->file_path) {
            uvhttp_free(ctx);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(ctx->file_path, file_path, path_len);
        ctx->file_path[path_len] = '\0';

        /* get output file descriptor */
        int fd_result = uv_fileno((uv_handle_t*)resp->client, &ctx->out_fd);
        if (fd_result < 0) {
            UVHTTP_LOG_ERROR("Failed to get client fd: %s",
                             uv_strerror(fd_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }

        /* openinputfile */
        ctx->in_fd = open(file_path, O_RDONLY);
        if (ctx->in_fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file for sendfile: %s", file_path);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_NOT_FOUND;
        }

        /* getMIMEtype */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type));

        char content_length[64];
        snprintf(content_length, sizeof(content_length), "%zu", file_size);

        uvhttp_response_set_status(resp, 200);
        uvhttp_response_set_header(resp, "Content-Type", mime_type);
        uvhttp_response_set_header(resp, "Content-Length", content_length);

        /* build response header data */
        char* header_data = NULL;
        size_t header_length = 0;
        uvhttp_error_t build_result =
            uvhttp_response_build_data(resp, &header_data, &header_length);
        if (build_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("Failed to build response headers: %s",
                             uvhttp_error_string(build_result));
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            return build_result;
        }

        /* send response headers (using send_raw, don't mark response as
         * complete) */
        uvhttp_error_t send_result = uvhttp_response_send_raw(
            header_data, header_length, resp->client, resp);
        uvhttp_free(header_data); /* release built response header data */

        if (send_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("Failed to send response headers: %s",
                             uvhttp_error_string(send_result));
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            return send_result;
        }

        /* performance optimization: enable TCP_CORK to optimize large file
         * transmission */
        int cork = 1;
        setsockopt(ctx->out_fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
        ctx->cork_enabled = 1;

        /* initialize timeout timer */
        int timer_result = uv_timer_init(loop, &ctx->timeout_timer);
        if (timer_result != 0) {
            UVHTTP_LOG_ERROR("Failed to init timeout timer: %s",
                             uv_strerror(timer_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        ctx->timeout_timer.data = ctx;

        /* start chunked sendfile (send config's chunk size each time) */
        size_t chunk_size = ctx->chunk_size;

        uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout,
                       ctx->timeout_ms, 0);

        int sendfile_result =
            uv_fs_sendfile(loop, &ctx->sendfile_req, ctx->out_fd, ctx->in_fd, 0,
                           chunk_size, on_sendfile_complete);

        /* check if sendfile failed synchronously */
        if (sendfile_result < 0) {
            UVHTTP_LOG_ERROR("Failed to start sendfile: %s",
                             uv_strerror(sendfile_result));
            /* clean resources */
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }

        return UVHTTP_OK;
    }
}

/* zero-copy send static file (mixed strategy) - use default config */
uvhttp_result_t uvhttp_static_sendfile(const char* file_path, void* response) {
    /* call internal function, use NULL config (use default value) */
    return uvhttp_static_sendfile_with_config(file_path, response, NULL);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
