/* UVHTTP - 超轻量级HTTP服务器库 - API版本 */

#ifndef UVHTTP_H
#define UVHTTP_H

#include "uvhttp_features.h"



/* 核心模块 */
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_utils.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"

/* 可选功能的条件包含 */


#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif

#if UVHTTP_FEATURE_TLS
#include "uvhttp_tls.h"
#endif

#if UVHTTP_FEATURE_STATIC_FILES
#include "uvhttp_static.h"
#endif

/* 可选功能 */
#if UVHTTP_FEATURE_ALLOCATOR
#include "uvhttp_allocator.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 版本信息 */
#define UVHTTP_VERSION_MAJOR 1
#define UVHTTP_VERSION_MINOR 0
#define UVHTTP_VERSION_PATCH 0
#define UVHTTP_VERSION_STRING "1.0.0"

/* API使用说明：
 * 
 * 核心API：
 *   uvhttp_server_t* server = uvhttp_server_new(loop);
 *   uvhttp_router_t* router = uvhttp_router_new();
 *   uvhttp_router_add_route(router, "/", handler);
 * 
 * 启动服务器：
 *   uvhttp_server_listen(server, "0.0.0.0", 8080);
 *   uv_run(loop, UV_RUN_DEFAULT);
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_H */