# UVHTTP Routing System Design Philosophy

## Document Information

- **Project**: UVHTTP
- **Version**: 2.2.0
- **Creation Date**: 2026-02-23
- **Document Type**: Design Philosophy & Technical Guide
- **Status**: Implemented

---

## Table of Contents

1. [Overview](#overview)
2. [Routing System Design Philosophy](#routing-system-design-philosophy)
3. [Routing Architecture](#routing-architecture)
4. [Routing Matching Strategies](#routing-matching-strategies)
5. [Performance Optimization](#performance-optimization)
6. [Business Scenario Applications](#business-scenario-applications)
7. [Best Practices](#best-practices)
8. [Common Questions](#common-questions)

---

## Overview

The UVHTTP routing system is the HTTP request dispatching mechanism. It supports static routes, parameter routes, method routes, and prefix routes.

### Core Features

- **High performance**: O(1) prefix matching, supporting 128+ routes
- **Flexibility**: supports static routes, parameter routes, and method routes
- **Memory optimization**: compact memory layout (128-byte nodes), CPU cache friendly
- **Ease of use**: clean API design, intuitive route definition
- **Extensibility**: supports custom fallback routes and static file routes

---

## Routing System Design Philosophy

### 1. Performance First

**Principle**: Route matching is on the critical path of every HTTP request and must be optimized.

**Practice**:

- **O(1) prefix matching**: uses a Trie data structure for fast prefix matching
- **CPU cache friendly**: route nodes are designed to be 128 bytes (2 cache lines), optimizing memory locality
- **Compact storage**: uses a compact child index array (48 bytes stores 12 children)
- **Zero dynamic allocation**: the route node pool is pre-allocated, avoiding runtime memory allocation

**Benefits**:

- Route matching latency < 1μs
- Supports 128+ routes with no performance degradation
- Minimized memory usage

**Example**:

```c
// Route node design - 128 bytes, 2 cache lines
typedef struct uvhttp_route_node {
    /* Cache line 1: Hot path fields (64 bytes) */
    uvhttp_method_t method;           /* 4 bytes */
    uvhttp_request_handler_t handler; /* 8 bytes */
    size_t child_count;               /* 8 bytes */
    int is_param;                     /* 4 bytes */
    uint8_t segment_len;              /* 1 byte */
    uint8_t param_name_len;           /* 1 byte */
    uint16_t _padding1;               /* 2 bytes */
    uint32_t child_indices[12];       /* 48 bytes - Compact child storage */

    /* Cache line 2: Variable length data (64 bytes) */
    char segment_data[32];    /* 32 bytes */
    char param_name_data[32]; /* 32 bytes */
} uvhttp_route_node_t;
```

### 2. Flexibility and Control

**Principle**: The application layer has full control over routing logic; the framework does not impose a routing pattern.

**Practice**:

- **Explicit route registration**: developers explicitly register each route
- **Method route support**: supports different HTTP method handlers for the same path
- **Parameter routes**: supports path parameter extraction (e.g. `/users/:id`)
- **Fallback routes**: supports custom handlers for unmatched routes
- **Static file routes**: the application layer implements static file routing strategy itself

**Benefits**:

- Full control over routing
- Flexible routing strategies
- Easy to extend and customize

**Example**:

```c
// Add routes
uvhttp_router_add_route(router, "/", home_handler);
uvhttp_router_add_route(router, "/about", about_handler);

// Add method routes
uvhttp_router_add_route_method(router, "/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, get_user_handler);
```

### 3. Simplicity and Intuitiveness

**Principle**: The API is designed to be simple and intuitive, lowering the learning cost.

**Practice**:

- **Unified API**: all routing operations use a unified API
- **Intuitive parameters**: path and handler are the core parameters
- **Optional features**: advanced features (such as parameter routes) are used on demand
- **Clear naming**: function names clearly express their functionality

**Benefits**:

- Fast onboarding
- Fewer errors
- Improved development efficiency

**Example**:

```c
// Create a router
uvhttp_router_t* router = NULL;
uvhttp_router_new(&router);

// Add a route
uvhttp_router_add_route(router, "/api/users", users_handler);

// Find a handler
uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api/users", "GET");

// Free the router
uvhttp_router_free(router);
```

### 4. Memory Safety

**Principle**: The routing system must be memory safe, with no leaks and no overflows.

**Practice**:

- **Buffer bounds checking**: all string operations have bounds checking
- **Memory pool management**: route nodes use a memory pool, avoiding fragmentation
- **Strict cleanup**: provides complete cleanup functions
- **Error handling**: all operations return error values

**Benefits**:

- No memory leaks
- No buffer overflows
- Stable for long-running operation

**Example**:

```c
// Safe path parameter extraction
uvhttp_error_t result = uvhttp_router_match(router, "/users/123", "GET", &match);
if (result == UVHTTP_OK) {
    // The number of parameters is limited (MAX_PARAMS = 16)
    for (size_t i = 0; i < match.param_count; i++) {
        printf("Parameter %s = %s\n", match.params[i].name, match.params[i].value);
    }
}
```

---

## Routing Architecture

### Router Structure

```c
typedef struct uvhttp_router {
    /* Hot path fields */
    int use_trie;                   /* Whether to use the Trie */
    size_t route_count;             /* Total number of routes */

    /* Trie routing related */
    uvhttp_route_node_t* node_pool; /* Node pool */
    uint32_t root_index;            /* Root node index */
    uint32_t node_pool_size;        /* Pool capacity */
    uint32_t node_pool_used;        /* Pool usage */

    /* Array routing related */
    array_route_t* array_routes;    /* Array routes */
    size_t array_route_count;       /* Number of array routes */
    size_t array_capacity;          /* Array capacity */

    /* Static file routing support */
    char* static_prefix;            /* Static file prefix */
    void* static_context;           /* Static file context */
    uvhttp_request_handler_t static_handler; /* Static file handler */

    /* Fallback routing support */
    void* fallback_context;         /* Fallback context */
    uvhttp_request_handler_t fallback_handler; /* Fallback handler */
} uvhttp_router_t;
```

### Route Node Structure

```c
typedef struct uvhttp_route_node {
    /* Cache line 1: Hot path fields (64 bytes) */
    uvhttp_method_t method;           /* HTTP method */
    uvhttp_request_handler_t handler; /* Request handler */
    size_t child_count;               /* Number of children */
    int is_param;                     /* Whether it is a parameter node */
    uint8_t segment_len;              /* Segment length */
    uint8_t param_name_len;           /* Parameter name length */
    uint16_t _padding1;               /* Padding */
    uint32_t child_indices[12];       /* Compact child indices */

    /* Cache line 2: Variable length data (64 bytes) */
    char segment_data[32];    /* Path segment data */
    char param_name_data[32]; /* Parameter name data */
} uvhttp_route_node_t;
```

### Route Matching Flow

```
1. Request arrives
   ↓
2. Extract path and method
   ↓
3. Traverse the Trie tree
   ├─ Static segment matching
   ├─ Parameter segment matching
   └─ Method matching
   ↓
4. Find the handler
   ↓
5. Extract path parameters
   ↓
6. Invoke the handler
```

---

## Routing Matching Strategies

### 1. Static Route Matching

**Description**: matches static paths exactly, with no parameters.

**Example**:

```c
// Add static routes
uvhttp_router_add_route(router, "/", home_handler);
uvhttp_router_add_route(router, "/about", about_handler);
uvhttp_router_add_route(router, "/api/status", status_handler);
```

**Matching rules**:

- Match the path exactly
- Case-insensitive (configurable)
- Higher priority than parameter routes

### 2. Parameter Route Matching

**Description**: supports path parameter extraction, such as `/users/:id`.

**Example**:

```c
// Add parameter routes
uvhttp_router_add_route(router, "/users/:id", user_handler);
uvhttp_router_add_route(router, "/posts/:post_id/comments/:comment_id", comment_handler);

// Extract parameters
uvhttp_route_match_t match;
uvhttp_router_match(router, "/users/123", "GET", &match);
// match.params[0].name = "id"
// match.params[0].value = "123"
```

**Matching rules**:

- Parameters start with `:`
- Parameter names are limited to 64 characters
- Parameter values are limited to 256 characters
- Supports up to 16 parameters

### 3. Method Route Matching

**Description**: sets different HTTP method handlers for the same path.

**Example**:

```c
// Add different method handlers for the same path
uvhttp_router_add_route_method(router, "/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, get_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_PUT, update_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_DELETE, delete_user_handler);
```

**Matching rules**:

- Match the path first, then the method
- Supports all HTTP methods (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
- Unmatched methods return 405 Method Not Allowed

### 4. Prefix Route Matching

**Description**: matches all paths beginning with a specified prefix.

**Example**:

```c
// Set a static file prefix
router->static_prefix = "/static";
router->static_handler = static_file_handler;

// All /static/* requests are handled by static_file_handler
```

**Matching rules**:

- Prefix matching has the lowest priority
- Commonly used for static file serving
- The application layer implements the matching logic itself

### 5. Fallback Route Matching

**Description**: the default handler used when no route matches.

**Example**:

```c
// Set a fallback handler
router->fallback_handler = not_found_handler;

// All unmatched requests are handled by not_found_handler
```

**Matching rules**:

- The last matching attempt
- Commonly used for 404 handling
- Returns 404 Not Found

---

## Performance Optimization

### 1. CPU Cache Optimization

**Technique**: route nodes are designed to be 128 bytes (2 cache lines).

**Benefits**:

- Fewer cache misses
- Faster route matching
- Optimized memory locality

### 2. Compact Storage

**Technique**: uses a compact child index array.

**Benefits**:

- Reduced memory usage
- Improved CPU cache utilization
- Supports more children

### 3. Zero Dynamic Allocation

**Technique**: the route node pool is pre-allocated.

**Benefits**:

- Avoids runtime memory allocation
- Reduces memory fragmentation
- Improves matching performance

### 4. Fast Prefix Matching

**Technique**: O(1) prefix matching algorithm.

**Benefits**:

- Fast matching of static routes
- Supports a large number of routes
- Stable performance

---

## Business Scenario Applications

### Scenario 1: RESTful API Design

**Requirement**: implement a complete RESTful API supporting CRUD operations.

**Routing strategy**:

```c
// Resource route design
// GET    /api/users          - list all users
// POST   /api/users          - create a user
// GET    /api/users/:id      - get user details
// PUT    /api/users/:id      - update a user
// DELETE /api/users/:id      - delete a user

// User-related routes
uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_GET, get_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_PUT, update_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_DELETE, delete_user_handler);

// Post-related routes
uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_GET, get_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_PUT, update_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_DELETE, delete_post_handler);

// Comment-related routes (nested resources)
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments", UVHTTP_GET, list_comments_handler);
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments", UVHTTP_POST, create_comment_handler);
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments/:id", UVHTTP_GET, get_comment_handler);
```

**Handler implementation**:

```c
// List all users
int list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Query the database
    // Return a JSON response
    cJSON* json = cJSON_CreateArray();
    // ... add user data
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}

// Get user details
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Extract the user ID
    uvhttp_route_match_t match;
    uvhttp_router_match(router, uvhttp_request_get_url(req), 
                       uvhttp_request_get_method(req), &match);
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    // Query the database
    // Return a JSON response
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "id", user_id);
    cJSON_AddStringToObject(json, "name", "John Doe");
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}
```

### Scenario 2: Microservice Gateway

**Requirement**: implement a microservice gateway that forwards requests to different backend services based on the path.

**Routing strategy**:

```c
// Service routes
// /api/users/*  -> user-service:8081
// /api/posts/*  -> post-service:8082
// /api/orders/* -> order-service:8083

// Add route handlers
uvhttp_router_add_route(router, "/api/users", user_service_proxy_handler);
uvhttp_router_add_route(router, "/api/posts", post_service_proxy_handler);
uvhttp_router_add_route(router, "/api/orders", order_service_proxy_handler);

// Set prefix routes (catch all sub-paths)
uvhttp_router_add_route(router, "/api/users/*", user_service_proxy_handler);
uvhttp_router_add_route(router, "/api/posts/*", post_service_proxy_handler);
uvhttp_router_add_route(router, "/api/orders/*", order_service_proxy_handler);
```

**Handler implementation**:

```c
// User service proxy
int user_service_proxy_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* url = uvhttp_request_get_url(req);
    const char* method = uvhttp_request_get_method(req);
    const char* body = uvhttp_request_get_body(req);
    
    // Forward the request to the user service
    // 1. Construct the target URL
    char target_url[512];
    snprintf(target_url, sizeof(target_url), "http://user-service:8081%s", url);
    
    // 2. Forward the request
    // 3. Get the response
    // 4. Return the response
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, proxy_response, proxy_response_len);
    
    return uvhttp_response_send(res);
}
```

### Scenario 3: Multi-Version API

**Requirement**: support multiple versions of an API, such as v1 and v2.

**Routing strategy**:

```c
// Version routes
// /api/v1/users -> v1 API
// /api/v2/users -> v2 API

// Add version routes
uvhttp_router_add_route(router, "/api/v1/users", v1_list_users_handler);
uvhttp_router_add_route(router, "/api/v2/users", v2_list_users_handler);

// Set a fallback route (default to v1)
router->fallback_handler = v1_fallback_handler;
```

**Handler implementation**:

```c
// v1 list users
int v1_list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // v1 API implementation
    cJSON* json = cJSON_CreateArray();
    // ... add user data
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}

// v2 list users (includes more information)
int v2_list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // v2 API implementation (includes more information)
    cJSON* json = cJSON_CreateArray();
    // ... add user data (includes more information)
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-API-Version", "v2");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}
```

### Scenario 4: Static File Serving

**Requirement**: provide static file serving, supporting multiple directories.

**Routing strategy**:

```c
// Static file routes
// /static/*      -> ./public/static/
// /uploads/*      -> ./public/uploads/
// /assets/*       -> ./public/assets/

// Set the static file prefix and handler
router->static_prefix = "/static";
router->static_handler = static_file_handler;

// Add specific static directory routes
uvhttp_router_add_route(router, "/uploads", uploads_handler);
uvhttp_router_add_route(router, "/assets", assets_handler);
```

**Handler implementation**:

```c
// Static file handler
int static_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* url = uvhttp_request_get_url(req);
    
    // Construct the file path
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "./public%s", url);
    
    // Check whether the file exists
    // If it exists, return the file
    // If not, return 404
    
    // Use uvhttp_static_handle_request to serve static files
    return uvhttp_static_handle_request(req, res, file_path);
}
```

### Scenario 5: Authentication and Authorization

**Requirement**: some routes require authentication and authorization.

**Routing strategy**:

```c
// Public routes
uvhttp_router_add_route(router, "/api/public/login", login_handler);
uvhttp_router_add_route(router, "/api/public/register", register_handler);

// Routes requiring authentication
uvhttp_router_add_route(router, "/api/users", auth_middleware_wrapper(list_users_handler));
uvhttp_router_add_route(router, "/api/posts", auth_middleware_wrapper(list_posts_handler));

// Routes requiring administrator privileges
uvhttp_router_add_route(router, "/api/admin/users", admin_middleware_wrapper(admin_list_users_handler));
```

**Middleware implementation**:

```c
// Authentication middleware wrapper
typedef int (*auth_middleware_wrapper_func)(uvhttp_request_t*, uvhttp_response_t*);

int auth_middleware_wrapper(auth_middleware_wrapper_func handler) {
    return wrapped_handler;
}

int wrapped_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 1. Extract the authentication token
    const char* auth_header = uvhttp_request_get_header(req, "Authorization");
    
    // 2. Verify the token
    if (!verify_token(auth_header)) {
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Unauthorized\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 3. Invoke the actual handler
    return handler(req, res);
}
```

---

## Best Practices

### 1. Route Organization

**Recommendation**: organize routes by functional module.

```c
// User module routes
void setup_user_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_GET, get_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_PUT, update_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_DELETE, delete_user_handler);
}

// Post module routes
void setup_post_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_GET, get_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_PUT, update_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_DELETE, delete_post_handler);
}

// Main function
int main() {
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    
    // Set up module routes
    setup_user_routes(router);
    setup_post_routes(router);
    
    // ...
}
```

### 2. Error Handling

**Recommendation**: use a unified error handling mechanism.

```c
// Unified error response
void send_error_response(uvhttp_response_t* res, int status, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", message);
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
}

// Usage example
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_route_match_t match;
    if (uvhttp_router_match(router, uvhttp_request_get_url(req), 
                           uvhttp_request_get_method(req), &match) != UVHTTP_OK) {
        send_error_response(res, 400, "Invalid request");
        return UVHTTP_OK;
    }
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    if (!user_id) {
        send_error_response(res, 400, "User ID is required");
        return UVHTTP_OK;
    }
    
    // ... process the request
}
```

### 3. Performance Optimization

**Recommendation**: prefer static routes first, parameter routes second.

```c
// Static routes take priority (better performance)
uvhttp_router_add_route(router, "/api/users/me", get_current_user_handler);
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);

// Avoid overusing parameter routes
// Not recommended
uvhttp_router_add_route(router, "/api/:resource/:id", generic_handler);

// Recommended (explicit for each resource)
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);
uvhttp_router_add_route(router, "/api/posts/:id", get_post_handler);
```

### 4. Security

**Recommendation**: validate all input parameters.

```c
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_route_match_t match;
    if (uvhttp_router_match(router, uvhttp_request_get_url(req), 
                           uvhttp_request_get_method(req), &match) != UVHTTP_OK) {
        send_error_response(res, 400, "Invalid request");
        return UVHTTP_OK;
    }
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    // Validate the user ID format
    if (!user_id || !is_valid_user_id(user_id)) {
        send_error_response(res, 400, "Invalid user ID");
        return UVHTTP_OK;
    }
    
    // ... process the request
}
```

---

## Common Questions

### Q1: How to implement wildcard routes?

**A**: UVHTTP does not support wildcard routes (such as `/api/*`). It is recommended to use prefix routes or static routes.

```c
// Not supported
// uvhttp_router_add_route(router, "/api/*", handler);

// Recommended: use a prefix route
router->static_prefix = "/api";
router->static_handler = api_handler;

// Or use specific routes
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);
```

### Q2: How to implement route priority?

**A**: Routes are matched in the order they are added; routes added earlier have higher priority.

```c
// Add the more specific route first
uvhttp_router_add_route(router, "/api/users/me", get_current_user_handler);
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);
```

### Q3: How to implement route grouping?

**A**: use functions to organize routes.

```c
void setup_user_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
}

void setup_post_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
}
```

### Q4: How to implement route redirects?

**A**: return a 301/302 status code in the handler.

```c
int redirect_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 301);
    uvhttp_response_set_header(res, "Location", "https://example.com/new-path");
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    const char* message = "Moved Permanently";
    uvhttp_response_set_body(res, message, strlen(message));
    
    return uvhttp_response_send(res);
}
```

### Q5: How to implement CORS?

**A**: add CORS headers in the handler.

```c
int cors_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Add CORS headers
    uvhttp_response_set_header(res, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(res, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(res, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // Handle OPTIONS requests
    const char* method = uvhttp_request_get_method(req);
    if (strcmp(method, "OPTIONS") == 0) {
        uvhttp_response_set_status(res, 200);
        return uvhttp_response_send(res);
    }
    
    // Handle other requests
    return actual_handler(req, res);
}
```

---

## Appendix

### A. Routing API Reference

| Function | Description |
|-----|------|
| `uvhttp_router_new` | Create a router |
| `uvhttp_router_free` | Free a router |
| `uvhttp_router_add_route` | Add a route (default GET method) |
| `uvhttp_router_add_route_method` | Add a route (specified method) |
| `uvhttp_router_find_handler` | Find a route handler |
| `uvhttp_router_match` | Match a route and extract parameters |
| `uvhttp_parse_path_params` | Parse path parameters |

### B. HTTP Method Enumeration

| Method | Enum Value | Description |
|-----|-------|------|
| GET | `UVHTTP_GET` | Retrieve a resource |
| POST | `UVHTTP_POST` | Create a resource |
| PUT | `UVHTTP_PUT` | Update a resource |
| DELETE | `UVHTTP_DELETE` | Delete a resource |
| PATCH | `UVHTTP_PATCH` | Partially update |
| HEAD | `UVHTTP_HEAD` | Retrieve headers |
| OPTIONS | `UVHTTP_OPTIONS` | Retrieve options |

### C. Routing Configuration Constants

| Constant | Value | Description |
|-----|---|------|
| `MAX_ROUTES` | 128 | Maximum number of routes |
| `MAX_ROUTE_PATH_LEN` | 256 | Maximum route path length |
| `MAX_PARAMS` | 16 | Maximum number of parameters |
| `MAX_PARAM_NAME_LEN` | 64 | Maximum parameter name length |
| `MAX_PARAM_VALUE_LEN` | 256 | Maximum parameter value length |

---

## Change History

| Date | Version | Changes | Author |
|-----|------|---------|------|
| 2026-02-23 | 1.0 | Initial version, complete routing system design philosophy | iFlow |

---

## Contact

If you have questions or suggestions, please contact us via:

- **GitHub Issues**: https://github.com/adam-ikari/uvhttp/issues
- **Email**: [To be added]
- **Slack**: [To be added]

---

**End of Document**
