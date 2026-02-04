/**
 * @file test_version_api.c
 * @brief Test program for version and build configuration API
 */

#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("========================================\n");
    printf("    UVHTTP Version API Test            \n");
    printf("========================================\n\n");

    /* Test version string */
    printf("Version String: %s\n", uvhttp_get_version_string());

    /* Test version components */
    int major, minor, patch;
    uvhttp_get_version(&major, &minor, &patch);
    printf("Version Components: %d.%d.%d\n", major, minor, patch);

    /* Test version as integer */
    printf("Version as Integer: %d\n", uvhttp_get_version_int());

    printf("\n");

    /* Test feature checking */
    printf("Feature Status:\n");
    printf("  WebSocket:       %s\n", uvhttp_is_feature_enabled("websocket") ? "Enabled" : "Disabled");
    printf("  Static Files:    %s\n", uvhttp_is_feature_enabled("static_files") ? "Enabled" : "Disabled");
    printf("  TLS:             %s\n", uvhttp_is_feature_enabled("tls") ? "Enabled" : "Disabled");
    printf("  Middleware:      %s\n", uvhttp_is_feature_enabled("middleware") ? "Enabled" : "Disabled");
    printf("  Logging:         %s\n", uvhttp_is_feature_enabled("logging") ? "Enabled" : "Disabled");
    printf("  Router Cache:    %s\n", uvhttp_is_feature_enabled("router_cache") ? "Enabled" : "Disabled");
    printf("  LRU Cache:       %s\n", uvhttp_is_feature_enabled("lru_cache") ? "Enabled" : "Disabled");
    printf("  CORS:            %s\n", uvhttp_is_feature_enabled("cors") ? "Enabled" : "Disabled");
    printf("  Rate Limit:      %s\n", uvhttp_is_feature_enabled("rate_limit") ? "Enabled" : "Disabled");
    printf("  Protocol Upgrade: %s\n", uvhttp_is_feature_enabled("protocol_upgrade") ? "Enabled" : "Disabled");

    printf("\n");

    /* Test allocator type */
    printf("Allocator Type: %s\n", uvhttp_get_allocator_type());

    /* Test build type */
    printf("Build Type: %s\n", uvhttp_get_build_type());

    /* Test compiler info */
    printf("Compiler: %s\n", uvhttp_get_compiler_info());

    /* Test platform info */
    printf("Platform: %s\n", uvhttp_get_platform_info());

    printf("\n");

    /* Test build info structure */
    uvhttp_build_info_t info;
    uvhttp_error_t err = uvhttp_get_build_info(&info);
    if (err == UVHTTP_OK) {
        printf("Build Info Structure:\n");
        printf("  Version: %s\n", info.version_string);
        printf("  Build Type: %s\n", info.build_type);
        printf("  Compiler: %s\n", info.compiler);
        printf("  Platform: %s\n", info.platform);
    } else {
        printf("Failed to get build info: %d\n", err);
    }

    printf("\n");

    /* Test JSON output */
    char* json_str = NULL;
    err = uvhttp_get_build_info_json(&json_str);
    if (err == UVHTTP_OK) {
        printf("Build Info JSON:\n");
        printf("%s\n", json_str);
        uvhttp_free(json_str);
    } else {
        printf("Failed to get build info JSON: %d\n", err);
    }

    printf("\n");

    /* Test print function */
    uvhttp_print_build_info();

    printf("\n========================================\n");
    printf("    All Tests Passed!                 \n");
    printf("========================================\n");

    return 0;
}
