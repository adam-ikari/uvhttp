#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_error_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

/* ========== 编译时验证 ========== */
/* 验证结构体大小，确保内存布局优化不会被破坏 */
#ifdef __cplusplus
#define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* 连接池管理 - 优化连接复用 */

