# Router API Spec

## Overview

The Router module maps HTTP request paths and methods to handler functions.
It supports two routing strategies: array-based (for small route sets) and
trie-based (for large route sets with prefix matching). Transition between
the two modes is automatic.

## Interfaces

### uvhttp_router_new
- **Signature**: `uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router)`
- **Purpose**: Create a new router instance
- **Preconditions**: `router` must be a non-NULL pointer to `uvhttp_router_t*`
- **Postconditions**: On success, `*router` points to a valid router with `route_count=0`, `use_trie=0`, `array_routes=NULL`, `node_pool=NULL`.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `router` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
- **Thread safety**: Not thread-safe.

### uvhttp_router_free
- **Signature**: `void uvhttp_router_free(uvhttp_router_t* router)`
- **Purpose**: Free all router resources
- **Preconditions**: `router` must be valid (from `uvhttp_router_new`). Can be NULL (no-op).
- **Postconditions**: All memory is freed. The router pointer is invalid after return.
- **Thread safety**: Not thread-safe.

### uvhttp_router_add_route
- **Signature**: `uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler)`
- **Purpose**: Add a route for all HTTP methods
- **Preconditions**: `router` must be valid. `path` must be non-NULL. `handler` must be non-NULL.
- **Postconditions**: The route is added to the router. If the route count exceeds the array threshold, automatic migration to trie mode occurs.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `router`, `path`, or `handler` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
  - `UVHTTP_ERROR_NOT_FOUND`: route count exceeds MAX_ROUTES
- **Thread safety**: Not thread-safe.

### uvhttp_router_add_route_method
- **Signature**: `uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router, const char* path, uvhttp_method_t method, uvhttp_request_handler_t handler)`
- **Purpose**: Add a route for a specific HTTP method
- **Preconditions**: Same as `uvhttp_router_add_route`, plus `method` must be a valid `uvhttp_method_t`.
- **Postconditions**: Same as `uvhttp_router_add_route`, but the route is only matched for the specified method.
- **Error conditions**: Same as `uvhttp_router_add_route`.
- **Thread safety**: Not thread-safe.

### uvhttp_router_find_handler
- **Signature**: `uvhttp_request_handler_t uvhttp_router_find_handler(const uvhttp_router_t* router, const char* path, const char* method)`
- **Purpose**: Find the handler for a given path and method
- **Preconditions**: `router` must be valid. `path` must be non-NULL. `method` must be a valid HTTP method string.
- **Postconditions**: Returns the matching handler, or NULL if no match is found.
- **Thread safety**: Thread-safe for reads after all routes are added.

### uvhttp_router_match
- **Signature**: `uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router, const char* path, const char* method, uvhttp_route_match_t* match)`
- **Purpose**: Match a path and extract path parameters
- **Preconditions**: Same as `uvhttp_router_find_handler`, plus `match` must be non-NULL.
- **Postconditions**: On success, `match->handler` is set, `match->params` contains extracted parameters, `match->param_count` is set.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: any argument is NULL
  - `UVHTTP_ERROR_NOT_FOUND`: no matching route
- **Thread safety**: Thread-safe for reads.

## Route Path Syntax

- `/users` — exact match
- `/users/:id` — parameter match (extracts `id` from path segment)
- `/static/*` — prefix match (matches any path starting with `/static/`)
- `/api/v1/users` — static prefix with multiple segments

## Behavior Rules

1. **Array mode**: Routes are stored in a flat array. Matching is O(n) linear scan. Used when route count is below the migration threshold.

2. **Trie mode**: Routes are stored in a compact prefix trie (128-byte nodes). Matching is O(k) where k is the path length. Used after automatic migration.

3. **Automatic migration**: When the route count exceeds the array threshold (default: 8), the array is migrated to a trie. Migration is transparent: all routes remain functional.

4. **Parameter extraction**: Path parameters (`:param`) are extracted during matching. Parameters are stored in `uvhttp_route_match_t::params`.

5. **Wildcard routes**: The `*` wildcard matches any path prefix. Wildcard routes have the lowest priority.

6. **Method matching**: When a route is added with a specific method, only requests with that method match. Routes added without a method match all methods.

7. **Fallback handler**: If set, the fallback handler is called when no route matches. The fallback handler is the last resort before returning 404.

## Performance Requirements

- Array mode matching: O(n) where n = route count
- Trie mode matching: O(k) where k = path segment count
- Route addition: O(1) amortized (array mode), O(k) (trie mode)
- Node size: 128 bytes (2 cache lines)
- Memory: ~128 bytes per route (trie mode), ~512 bytes per route (array mode)
- Migration: automatic, transparent

## Test Requirements

- Route addition (all methods, specific methods, wildcard, parameter)
- Route matching (exact, prefix, parameter, method-specific)
- Non-matching routes return NULL handler
- Array-to-trie migration
- NULL parameter handling for all public functions
- Maximum route count enforcement
- Parameter extraction correctness
- Fallback handler behavior
- Static file route registration
- Memory cleanup (no leaks on free)