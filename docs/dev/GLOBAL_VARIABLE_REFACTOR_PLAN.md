# Global Variable Refactor Plan

## Overview

The UVHTTP project contains static global variables that violate the project's "avoid global variables" principle. This document records the plan to refactor these global variables to use the loop->data pattern.

## Current Global Variables

### 1. `g_tls_initialized` (uvhttp_tls.c)
```c
static int g_tls_initialized = 0;
```
**Purpose**: Tracks whether the TLS module has been initialized, to avoid repeated initialization
**Impact**: TLS connection creation
**Priority**: Medium

### 2. `g_drbg_initialized` (uvhttp_websocket_native.c)
```c
static int g_drbg_initialized = 0;
```
**Purpose**: Tracks whether the DRBG (Deterministic Random Bit Generator) has been initialized
**Impact**: WebSocket connection creation
**Priority**: Medium

### 3. `error_stats` (uvhttp_error.c)
```c
static uvhttp_error_stats_t error_stats = {0};
```
**Purpose**: Records error statistics
**Impact**: Error monitoring and debugging
**Priority**: Low

### 4. `g_config_callback` (uvhttp_config.c)
```c
static uvhttp_config_change_callback_t g_config_callback = NULL;
```
**Purpose**: Configuration change callback function
**Impact**: Configuration hot reload and monitoring
**Priority**: Medium

### 5. `g_current_config` (uvhttp_config.c)
```c
static uvhttp_config_t* g_current_config = NULL;
```
**Purpose**: The currently active configuration
**Impact**: All configuration operations
**Priority**: High

## Refactor Strategy

### Phase 1: Extend the Server Context Struct

```c
typedef struct {
    /* Existing fields */
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    
    /* TLS state */
    int tls_initialized;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    /* WebSocket state */
    int drbg_initialized;
    
    /* Error statistics */
    uvhttp_error_stats_t error_stats;
    
    /* Configuration management */
    uvhttp_config_t* current_config;
    uvhttp_config_change_callback_t config_callback;
} uvhttp_app_context_t;
```

### Phase 2: Modify API Signatures

#### TLS Module
```c
// Old API
uvhttp_tls_error_t uvhttp_tls_init(void);

// New API
uvhttp_tls_error_t uvhttp_tls_init(uvhttp_app_context_t* ctx);
```

#### WebSocket Module
```c
// Old API
uvhttp_error_t uvhttp_websocket_init(void);

// New API
uvhttp_error_t uvhttp_websocket_init(uvhttp_app_context_t* ctx);
```

#### Error Module
```c
// Old API
void uvhttp_record_error(uvhttp_error_t error, const char* context);

// New API
void uvhttp_record_error(uvhttp_app_context_t* ctx, uvhttp_error_t error, const char* context);
```

#### Configuration Module
```c
// Old API
int uvhttp_config_monitor_changes(uvhttp_config_change_callback_t callback);
int uvhttp_config_reload(void);

// New API
int uvhttp_config_monitor_changes(uvhttp_app_context_t* ctx, uvhttp_config_change_callback_t callback);
int uvhttp_config_reload(uvhttp_app_context_t* ctx);
```

### Phase 3: Update All Call Sites

All calls in the following modules need to be updated:
- `uvhttp_server.c` - server initialization and TLS initialization
- `uvhttp_websocket_native.c` - WebSocket connection creation
- `uvhttp_error.c` - error recording
- `uvhttp_config.c` - configuration management
- `uvhttp_tls.c` - TLS initialization

### Phase 4: Update Tests

All tests that use these global variables need to be updated to pass the context.

### Phase 5: Update Documentation

Update the following documents:
- API_REFERENCE.md
- DEVELOPER_GUIDE.md
- LIBUV_DATA_POINTER.md
- Example code

## Implementation Steps

1. **Create a branch**: `refactor/global-variable-elimination`
2. **Extend the context struct**: add all global variable fields
3. **Modify the API**: change the API signature for each module incrementally
4. **Update call sites**: update all places using the old API
5. **Update tests**: ensure all tests pass
6. **Update documentation**: update API docs and examples
7. **Performance tests**: ensure performance does not regress
8. **Code review**: submit a PR for review
9. **Merge**: merge into the main branch

## Risk Assessment

| Risk | Level | Mitigation |
|------|------|---------|
| Breaking API change | 🔴 High | Major version bump, provide migration guide |
| Performance regression | 🟡 Medium | Performance tests, optimize hot spots |
| Test failure | 🟡 Medium | Update tests incrementally, ensure coverage |
| Documentation inconsistency | 🟢 Low | Update documentation synchronously |

## Expected Benefits

1. **Conforms to project standards**: eliminate global variables, use the loop->data pattern
2. **Multi-instance support**: support running multiple server instances in the same process
3. **Better testing**: unit tests are easier to isolate and manage
4. **Cloud-native friendly**: better suited to containerized and microservice architectures

## Time Estimate

- Phases 1-2: 4-6 hours
- Phases 3-4: 8-12 hours
- Phases 5-6: 4-6 hours
- Phases 7-9: 4-6 hours

**Total**: 20-30 hours

## Version Plan

It is recommended to implement this refactor in version **v2.0.0** because it is a breaking API change.

## References

- [LIBUV_DATA_POINTER.md](../guide/LIBUV_DATA_POINTER.md) - libuv data pointer pattern guide
- [DEVELOPER_GUIDE.md](../guide/DEVELOPER_GUIDE.md) - contributor guide
- [API_REFERENCE.md](../../api/API_REFERENCE.md) - API reference
