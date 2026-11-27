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
#include "uvhttp_version.h"

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

/* Memory allocator - always included */
#include "uvhttp_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* API usage instructions:
 *
 * One atomic call creates the server, builds its router and starts listening.
 * All-or-nothing: *server stays NULL on any failure, so there is no rollback
 * code and no moment where ownership of the router is ambiguous.
 *
 *   uvhttp_route_t routes[] = {{"/", UVHTTP_ANY, handler}};
 *   uvhttp_server_t* server = NULL;
 *   uvhttp_error_t err = uvhttp_server_listen_routes(
 *       loop, routes, 1, "0.0.0.0", 8080, &server);
 *   if (err != UVHTTP_OK) { ... }
 *   uv_run(loop, UV_RUN_DEFAULT);
 *
 * The primitive calls (server_new / router_new / add_route / take_router /
 * listen) remain available when the router must be assembled dynamically.
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_H */