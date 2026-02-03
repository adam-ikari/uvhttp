/**
 * @file uvhttp_protocol_upgrade.h
 * @brief UVHTTP protocol upgrade framework
 *
 * This file provides interfaces for HTTP protocol upgrade to custom protocols
 * (such as IPPS, gRPC-Web, etc.).
 *
 * @copyright Copyright (c) 2026
 * @license MIT License
 */

#ifndef UVHTTP_PROTOCOL_UPGRADE_H
#define UVHTTP_PROTOCOL_UPGRADE_H

#include "uvhttp_common.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Function pointer type definitions ========== */

/**
 * @brief Protocol detector function type
 *
 * This function is used to detect if an HTTP request matches a specific
 * protocol.
 *
 * @param request HTTP request object
 * @param protocol_name Output parameter, detected protocol name
 * @param protocol_name_len Protocol name buffer size
 * @return int 1 if protocol detected, 0 if not detected
 */
typedef int (*uvhttp_protocol_detector_t)(uvhttp_request_t* request,
                                          char* protocol_name,
                                          size_t protocol_name_len);

/**
 * @brief Protocol upgrade handler function type
 *
 * This function is used to handle protocol upgrade logic.
 *
 * @param conn HTTP connection object
 * @param protocol_name Protocol name
 * @param user_data User data (passed during registration)
 * @return uvhttp_error_t UVHTTP_OK for success, other values for errors
 */
typedef uvhttp_error_t (*uvhttp_protocol_upgrade_handler_t)(
    uvhttp_connection_t* conn, const char* protocol_name, void* user_data);

/**
 * @brief Connection ownership transfer callback function type
 *
 * This callback is called when connection ownership is transferred.
 *
 * @param tcp_handle TCP handle
 * @param fd File descriptor
 * @param user_data User data
 */
typedef void (*uvhttp_connection_ownership_callback_t)(uv_tcp_t* tcp_handle,
                                                       int fd, void* user_data);

/**
 * @brief Connection lifecycle callback structure
 *
 * Used to set callback function when connection is closed.
 */
typedef struct {
    void* user_data; /**< User data */
    void (*on_close)(
        void* user_data); /**< Callback function when connection is closed */
} uvhttp_connection_lifecycle_t;

/* ========== Internal type definitions ========== */

/**
 * @brief Protocol information structure (internal use)
 */
typedef struct uvhttp_protocol_info {
    char name[32];                             /**< Protocol name */
    char upgrade_header[64];                   /**< Upgrade header value */
    uvhttp_protocol_detector_t detector;       /**< Protocol detector */
    uvhttp_protocol_upgrade_handler_t handler; /**< Upgrade handler */
    void* user_data;                           /**< User data */
    struct uvhttp_protocol_info* next;         /**< Linked list pointer */
} uvhttp_protocol_info_t;

/**
 * @brief Protocol registry structure (internal use)
 */
typedef struct {
    uvhttp_protocol_info_t* protocols; /**< Protocol linked list */
    size_t protocol_count;             /**< Protocol count */
} uvhttp_protocol_registry_t;

/* ========== Protocol registration API ========== */

/**
 * @brief Register protocol upgrade handler
 *
 * @param server Server object
 * @param protocol_name Protocol name (e.g., "ipps", "grpc-web")
 * @param upgrade_header Upgrade header value (optional, for fast matching)
 * @param detector Protocol detector function
 * @param handler Protocol upgrade handler function
 * @param user_data User data, will be passed to handler
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_server_register_protocol_upgrade(
    uvhttp_server_t* server, const char* protocol_name,
    const char* upgrade_header, uvhttp_protocol_detector_t detector,
    uvhttp_protocol_upgrade_handler_t handler, void* user_data);

/**
 * @brief Unregister protocol upgrade handler
 *
 * @param server Server object
 * @param protocol_name Protocol name
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_server_unregister_protocol_upgrade(
    uvhttp_server_t* server, const char* protocol_name);

/* ========== Connection ownership transfer API ========== */

/**
 * @brief Transfer connection ownership to external library
 *
 * This function will:
 * 1. Stop HTTP reading
 * 2. Stop timeout timer
 * 3. Get TCP file descriptor
 * 4. Mark connection as upgraded
 * 5. Call callback to transfer ownership
 *
 * @param conn HTTP connection object
 * @param callback Ownership transfer callback function
 * @param user_data User data
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn, uvhttp_connection_ownership_callback_t callback,
    void* user_data);

/**
 * @brief Set connection lifecycle callback
 *
 * Lifecycle callback will be called when connection is closed,
 * used to notify external library to clean up resources.
 *
 * @param conn HTTP connection object
 * @param lifecycle Lifecycle callback structure
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_connection_set_lifecycle(
    uvhttp_connection_t* conn, uvhttp_connection_lifecycle_t* lifecycle);

/* ========== Helper functions ========== */

/**
 * @brief Get connection file descriptor
 *
 * @param conn HTTP connection object
 * @param fd Output parameter, file descriptor
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_connection_get_fd(uvhttp_connection_t* conn, int* fd);

/**
 * @brief Get client address
 *
 * @param conn HTTP connection object
 * @param addr Output parameter, client address
 * @param addr_len Input/output parameter, address length
 * @return uvhttp_error_t UVHTTP_OK for success
 */
uvhttp_error_t uvhttp_connection_get_peer_address(uvhttp_connection_t* conn,
                                                  struct sockaddr_storage* addr,
                                                  socklen_t* addr_len);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_PROTOCOL_UPGRADE_H */