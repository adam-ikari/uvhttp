# Error API Spec

## Overview

The Error module provides a unified error code system and error handling
utilities. All public API functions return `uvhttp_error_t` codes.

## Error Code Hierarchy

```
UVHTTP_OK (0)
├── General Errors (-1 to -9)
│   ├── UVHTTP_ERROR_INVALID_PARAM (-1)
│   ├── UVHTTP_ERROR_OUT_OF_MEMORY (-2)
│   ├── UVHTTP_ERROR_NOT_FOUND (-3)
│   ├── UVHTTP_ERROR_NULL_POINTER (-5)
│   └── UVHTTP_ERROR_TIMEOUT (-7)
├── Server Errors (-100 to -106)
│   ├── UVHTTP_ERROR_SERVER_INIT (-100)
│   ├── UVHTTP_ERROR_SERVER_LISTEN (-101)
│   └── UVHTTP_ERROR_CONNECTION_LIMIT (-103)
├── Connection Errors (-200 to -207)
│   ├── UVHTTP_ERROR_CONNECTION_INIT (-200)
│   └── UVHTTP_ERROR_CONNECTION_TIMEOUT (-205)
├── Request/Response Errors (-300 to -309)
│   ├── UVHTTP_ERROR_REQUEST_INIT (-300)
│   ├── UVHTTP_ERROR_RESPONSE_SEND (-302)
│   ├── UVHTTP_ERROR_HEADER_TOO_LARGE (-305)
│   └── UVHTTP_ERROR_BODY_TOO_LARGE (-306)
├── TLS Errors (-400 to -418)
│   ├── UVHTTP_ERROR_TLS_INIT (-400)
│   └── UVHTTP_ERROR_TLS_HANDSHAKE (-402)
├── Router Errors (-500 to -504)
│   └── UVHTTP_ERROR_ROUTE_NOT_FOUND (-500)
├── Rate Limit Error (-550)
│   └── UVHTTP_ERROR_RATE_LIMIT_EXCEEDED (-550)
└── WebSocket Errors (-700 to -707)
    ├── UVHTTP_ERROR_WS_HANDSHAKE (-700)
    └── UVHTTP_ERROR_WS_FRAME_INVALID (-702)
```

## Interfaces

### uvhttp_error_string
- **Signature**: `const char* uvhttp_error_string(uvhttp_error_t error)`
- **Purpose**: Get a human-readable error string
- **Preconditions**: None
- **Returns**: A non-NULL string describing the error.
- **Thread safety**: Thread-safe (returns static strings).

### uvhttp_error_category_string
- **Signature**: `const char* uvhttp_error_category_string(uvhttp_error_t error)`
- **Purpose**: Get the error category (e.g., "Server", "Connection")
- **Preconditions**: None
- **Returns**: A non-NULL string with the category name.
- **Thread safety**: Thread-safe.

### uvhttp_error_description
- **Signature**: `const char* uvhttp_error_description(uvhttp_error_t error)`
- **Purpose**: Get a detailed error description
- **Preconditions**: None
- **Returns**: A non-NULL string with the description.
- **Thread safety**: Thread-safe.

### uvhttp_error_suggestion
- **Signature**: `const char* uvhttp_error_suggestion(uvhttp_error_t error)`
- **Purpose**: Get a suggested fix for the error
- **Preconditions**: None
- **Returns**: A non-NULL string with the suggestion.
- **Thread safety**: Thread-safe.

### uvhttp_error_is_recoverable
- **Signature**: `int uvhttp_error_is_recoverable(uvhttp_error_t error)`
- **Purpose**: Check if the error is recoverable
- **Preconditions**: None
- **Returns**: 1 if recoverable, 0 if fatal.
- **Thread safety**: Thread-safe.

## Behavior Rules

1. **All functions return strings**: Error string functions always return a non-NULL string, even for unknown error codes.

2. **Unknown error codes**: If an error code is not in the known range, the functions return a generic "Unknown error" message.

3. **Error codes are negative**: All error codes are negative integers. `UVHTTP_OK` (0) is the only non-error value.

4. **TLS "want read/write"**: TLS functions may return positive values (1, 2) to indicate "want read" or "want write" — these are not errors.

## Test Requirements

- All error codes have non-NULL string representations
- Category strings are non-NULL
- Description strings are non-NULL
- Suggestion strings are non-NULL
- Recoverable check is correct for each error type
- Unknown error codes return generic messages
- NULL parameter handling for error helper functions