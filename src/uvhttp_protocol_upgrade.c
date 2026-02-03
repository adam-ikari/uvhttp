/**
 * @file uvhttp_protocol_upgrade.c
 * @brief UVHTTP protocol upgrade framework implementation
 *
 * @copyright Copyright (c) 2026
 * @license MIT License
 */

#include "uvhttp_protocol_upgrade.h"

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_logging.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <string.h>

/* ========== Protocol registration API implementation ========== */

uvhttp_error_t uvhttp_server_register_protocol_upgrade(
    uvhttp_server_t* server, const char* protocol_name,
    const char* upgrade_header, uvhttp_protocol_detector_t detector,
    uvhttp_protocol_upgrade_handler_t handler, void* user_data) {

    if (!server || !protocol_name || !detector || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Check protocol name length */
    if (strlen(protocol_name) >= 32) {
        UVHTTP_LOG_ERROR("Protocol name too long: %s", protocol_name);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Check Upgrade header length */
    if (upgrade_header && strlen(upgrade_header) >= 64) {
        UVHTTP_LOG_ERROR("Upgrade header too long: %s", upgrade_header);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Create protocol registry (if not exists) */
    if (!server->protocol_registry) {
        uvhttp_protocol_registry_t* registry =
            uvhttp_alloc(sizeof(uvhttp_protocol_registry_t));
        if (!registry) {
            UVHTTP_LOG_ERROR("Failed to allocate protocol registry");
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        memset(registry, 0, sizeof(uvhttp_protocol_registry_t));
        server->protocol_registry = registry;
    }

    uvhttp_protocol_registry_t* registry =
        (uvhttp_protocol_registry_t*)server->protocol_registry;

    /* Check if protocol already registered */
    uvhttp_protocol_info_t* existing = registry->protocols;
    while (existing) {
        if (strcasecmp(existing->name, protocol_name) == 0) {
            UVHTTP_LOG_WARN("Protocol already registered: %s", protocol_name);
            return UVHTTP_ERROR_ALREADY_EXISTS;
        }
        existing = existing->next;
    }

    /* Check protocol count limit */
    if (registry->protocol_count >= 10) {
        UVHTTP_LOG_WARN("Too many protocols registered (max 10), performance "
                        "may be affected");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Create protocol info */
    uvhttp_protocol_info_t* info = uvhttp_alloc(sizeof(uvhttp_protocol_info_t));
    if (!info) {
        UVHTTP_LOG_ERROR("Failed to allocate protocol info");
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize protocol info */
    memset(info, 0, sizeof(uvhttp_protocol_info_t));
    strncpy(info->name, protocol_name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    if (upgrade_header) {
        strncpy(info->upgrade_header, upgrade_header,
                sizeof(info->upgrade_header) - 1);
        info->upgrade_header[sizeof(info->upgrade_header) - 1] = '\0';
    }
    info->detector = detector;
    info->handler = handler;
    info->user_data = user_data;
    info->next = NULL;

    info->next = registry->protocols;
    registry->protocols = info;
    registry->protocol_count++;

    UVHTTP_LOG_INFO("Registered protocol upgrade: %s", protocol_name);

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_unregister_protocol_upgrade(
    uvhttp_server_t* server, const char* protocol_name) {

    if (!server || !protocol_name) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!server->protocol_registry) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    uvhttp_protocol_registry_t* registry =
        (uvhttp_protocol_registry_t*)server->protocol_registry;

    /* Find and remove protocol */
    uvhttp_protocol_info_t* prev = NULL;
    uvhttp_protocol_info_t* current = registry->protocols;

    while (current) {
        if (strcasecmp(current->name, protocol_name) == 0) {
            /* Found protocol, remove from linked list */
            if (prev) {
                prev->next = current->next;
            } else {
                registry->protocols = current->next;
            }

            registry->protocol_count--;
            uvhttp_free(current);

            UVHTTP_LOG_INFO("Unregistered protocol upgrade: %s", protocol_name);

            return UVHTTP_OK;
        }

        prev = current;
        current = current->next;
    }

    UVHTTP_LOG_WARN("Protocol not found: %s", protocol_name);
    return UVHTTP_ERROR_NOT_FOUND;
}

/* ========== Connection ownership transfer API implementation ========== */

uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn, uvhttp_connection_ownership_callback_t callback,
    void* user_data) {

    if (!conn || !callback) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Validate connection state */
    if (conn->state != UVHTTP_CONN_STATE_HTTP_PROCESSING) {
        UVHTTP_LOG_ERROR("Invalid connection state for ownership transfer: %d",
                         conn->state);
        return UVHTTP_ERROR_CONNECTION_INIT;
    }

    /* Check if already upgraded */
    if (conn->state == UVHTTP_CONN_STATE_PROTOCOL_UPGRADED) {
        UVHTTP_LOG_ERROR("Connection already upgraded");
        return UVHTTP_ERROR_CONNECTION_INIT;
    }

    /* Stop HTTP reading */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);

    /* Stop timeout timer */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* Get file descriptor */
    int fd = 0;
    int result = uv_fileno((uv_handle_t*)&conn->tcp_handle, &fd);
    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to get file descriptor");
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* Validate file descriptor */
    if (fd < 0) {
        UVHTTP_LOG_ERROR("Invalid file descriptor: %d", fd);
        return UVHTTP_ERROR_CONNECTION_INIT;
    }

    /* Mark connection as upgraded */
    conn->state = UVHTTP_CONN_STATE_PROTOCOL_UPGRADED;

    /* Call callback to transfer ownership */
    callback(&conn->tcp_handle, fd, user_data);

    UVHTTP_LOG_DEBUG("Connection ownership transferred, fd=%d", fd);

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_connection_set_lifecycle(
    uvhttp_connection_t* conn, uvhttp_connection_lifecycle_t* lifecycle) {

    if (!conn || !lifecycle) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Allocate lifecycle callback structure */
    if (!conn->lifecycle) {
        conn->lifecycle = uvhttp_alloc(sizeof(uvhttp_connection_lifecycle_t));
        if (!conn->lifecycle) {
            UVHTTP_LOG_ERROR("Failed to allocate lifecycle structure");
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
    }

    /* Copy lifecycle callbacks */
    memcpy(conn->lifecycle, lifecycle, sizeof(uvhttp_connection_lifecycle_t));

    return UVHTTP_OK;
}

/* ========== Helper function implementation ========== */

uvhttp_error_t uvhttp_connection_get_fd(uvhttp_connection_t* conn, int* fd) {

    if (!conn || !fd) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    int result = uv_fileno((uv_handle_t*)&conn->tcp_handle, fd);
    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to get file descriptor");
        return UVHTTP_ERROR_IO_ERROR;
    }

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_connection_get_peer_address(uvhttp_connection_t* conn,
                                                  struct sockaddr_storage* addr,
                                                  socklen_t* addr_len) {

    if (!conn || !addr || !addr_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    int len = (int)*addr_len;
    int result =
        uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)addr, &len);
    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to get peer address");
        return UVHTTP_ERROR_IO_ERROR;
    }

    *addr_len = (socklen_t)len;

    return UVHTTP_OK;
}