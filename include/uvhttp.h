/* UVHTTP - Ultra-lightweight HTTP server library - API version */

#ifndef UVHTTP_H
#define UVHTTP_H

#include "uvhttp_features.h"

/* Core modules */
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"

/* Conditional includes for optional features */

#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"
#endif

#if UVHTTP_FEATURE_TLS
#    include "uvhttp_tls.h"
#endif

#if UVHTTP_FEATURE_STATIC_FILES
#    include "uvhttp_static.h"
#endif

/* Optional features */
#if UVHTTP_FEATURE_ALLOCATOR
#    include "uvhttp_allocator.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* API usage instructions:
 *
 * Core API:
 *   uvhttp_server_t* server = uvhttp_server_new(loop);
 *   uvhttp_router_t* router = uvhttp_router_new();
 *   uvhttp_router_add_route(router, "/", handler);
 *
 * Start server:
 *   uvhttp_server_listen(server, "0.0.0.0", 8080);
 *   uv_run(loop, UV_RUN_DEFAULT);
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_H */