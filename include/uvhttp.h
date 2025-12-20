/* UVHTTP - 超轻量级HTTP服务器库 */

#ifndef UVHTTP_H
#define UVHTTP_H

#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_utils.h"
#include "uvhttp_json.h"
#include "uvhttp_middleware.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_websocket.h"
#include "uvhttp_http2.h"

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