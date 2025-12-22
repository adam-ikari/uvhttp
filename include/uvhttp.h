/* UVHTTP - 超轻量级HTTP服务器库 */

#ifndef UVHTTP_H
#define UVHTTP_H

#include "uvhttp_features.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_utils.h"

/* 可选功能的条件包含 */
#if UVHTTP_FEATURE_JSON
#include "uvhttp_json.h"
#endif

#if UVHTTP_FEATURE_ALLOCATOR
#include "uvhttp_allocator.h"
#endif

#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"

#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif

#if UVHTTP_FEATURE_TLS
#include "uvhttp_tls.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 版本信息 */
#define UVHTTP_VERSION_MAJOR 1
#define UVHTTP_VERSION_MINOR 0
#define UVHTTP_VERSION_PATCH 0
#define UVHTTP_VERSION_STRING "1.0.0"

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_H */