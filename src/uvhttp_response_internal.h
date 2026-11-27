#ifndef UVHTTP_RESPONSE_INTERNAL_H
#define UVHTTP_RESPONSE_INTERNAL_H

/*
 * Internal, incremental response-building primitives.
 *
 * These are deliberately NOT declared in include/uvhttp_response.h: the public
 * entry points are uvhttp_respond / uvhttp_respond_with_headers and the
 * no-send form uvhttp_response_prepare. The primitives stay visible here for
 * the library's own incremental construction paths — gzip header appending in
 * uvhttp_response.c, static-file multi-header assembly, and rollback inside
 * the atomic entries — and for white-box unit tests that link SOURCES.
 *
 * External users must not include this header. Symbols are kept under their
 * original names so existing library call sites and tests stay binary- and
 * source-compatible with the implementation in uvhttp_response.c.
 */

#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code);

uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                          const char* name, const char* value);

uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body, size_t length);

uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_RESPONSE_INTERNAL_H */
