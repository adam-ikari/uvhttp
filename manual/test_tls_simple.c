/*
 * Simple TLS test to diagnose the issue
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    int port = 8443;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    uvhttp_error_t result;
    
    /* Create loop */
    uv_loop_t* loop = uv_default_loop();
    
    /* Create server */
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        return 1;
    }
    printf("Server created successfully\n");
    
    /* Create TLS context */
    uvhttp_tls_context_t* tls_ctx = NULL;
    result = uvhttp_tls_context_new(&tls_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create TLS context: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_server_free(server);
        return 1;
    }
    printf("TLS context created successfully\n");
    
    /* Load certificates */
    result = uvhttp_tls_context_load_cert_chain(tls_ctx, "../test/certs/server.crt");
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load certificate: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    printf("Certificate loaded successfully\n");
    
    result = uvhttp_tls_context_load_private_key(tls_ctx, "../test/certs/server.key");
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load private key: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    printf("Private key loaded successfully\n");
    
    result = uvhttp_tls_context_load_ca_file(tls_ctx, "../test/certs/ca.crt");
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load CA certificate: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    printf("CA certificate loaded successfully\n");
    
    /* Enable TLS on server */
    result = uvhttp_server_enable_tls(server, tls_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to enable TLS: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    printf("TLS enabled successfully\n");
    
    /* Listen on port */
    printf("Attempting to listen on %s:%d...\n", host, port);
    result = uvhttp_server_listen(server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        uvhttp_server_free(server);
        uvhttp_tls_context_free(tls_ctx);
        return 1;
    }
    printf("Server listening successfully on %s:%d\n", host, port);
    
    /* Cleanup and exit */
    printf("Cleaning up...\n");
    uvhttp_server_free(server);
    uvhttp_tls_context_free(tls_ctx);
    
    printf("Test completed successfully\n");
    return 0;
}