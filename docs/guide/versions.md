---
title: Versions & Compatibility
description: UVHTTP version history and compatibility — current version 2.5.1, support policy, upgrade guide (including the 2.4→2.5 API migration to output-parameter constructors and the ASan/UBSan memory-safety verification).
---

# Versions

UVHTTP versions and compatibility.

## Current Version

**Version**: 2.5.1  
**Release Date**: 2026-07-25  
**Status**: Stable

## Version History

See [CHANGELOG](./CHANGELOG.md) for the complete version history.
## Compatibility

### Platform Support

| Platform | Version | Status |
|----------|---------|--------|
| Linux x86_64 | 2.2.0+ | ✅ Stable |
| Linux i386 | 2.2.0+ | ✅ Stable |
| macOS x86_64 | 2.2.0+ | ✅ Stable |
| macOS ARM64 | 2.2.0+ | ✅ Stable |
| Windows x86_64 | 2.2.0+ | ⚠️ Experimental |

### Compiler Support

| Compiler | Version | Status |
|----------|---------|--------|
| GCC | 4.8+ | ✅ Stable |
| Clang | 3.4+ | ✅ Stable |
| MSVC | 2019+ | ⚠️ Experimental |

### Dependency Versions

| Dependency | Version | Status |
|------------|---------|--------|
| libuv | 1.44.0+ | ✅ Required |
| llhttp | 8.1.0+ | ✅ Required |
| mbedtls | 3.0.0+ | ✅ Optional (TLS) |
| mimalloc | 2.0.0+ | ✅ Optional (Allocator) |
| cjson | 1.7.0+ | ✅ Optional (JSON) |

## Upgrade Guide

### From 1.x to 2.0

**Breaking Changes**:
- All function names changed
- New error handling system
- Different initialization process

**Migration Steps**:

1. Update function names:
```c
// Old
server_new(loop);
router_add_route(router, "/api", handler);

// New
uvhttp_server_new(loop);
uvhttp_router_add_route(router, "/api", handler);
```

2. Update error handling:
```c
// Old
if (server == NULL) {
    // Handle error
}

// New
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
}
```

3. Update initialization:
```c
// Old
uvhttp_server_t* server = server_new(loop);

// New
uvhttp_server_t* server = NULL;
uvhttp_server_new(loop, &server);
uvhttp_router_t* router = NULL;
uvhttp_router_new(&router);
uvhttp_server_set_router(server, router);
```

### From 2.0 to 2.1

**New Features**:
- WebSocket support
- Static file serving
- Rate limiting

**Migration Steps**:

No breaking changes. New features are opt-in via compile flags:

```bash
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

### From 2.1 to 2.2

**Breaking Changes**:

1. **TLS Error Type Integration** (2.2.1)
   - All TLS API functions now return `uvhttp_error_t` instead of `uvhttp_tls_error_t`
   - Error codes have been integrated into the unified error system

   **Migration**:
   ```c
   // Old (2.1.x)
   uvhttp_tls_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_TLS_OK) { /* handle error */ }
   
   // New (2.2.x)
   uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_OK) { /* handle error */ }
   ```

2. **Router Cache API Changes** (2.2.2)
   - Router cache optimization is now enabled by default
   - New router cache statistics available

   **Migration**:
   ```c
   // No code changes required
   // Router cache is automatically enabled
   // To disable: define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 0
   ```

**New Features**:
- Router cache optimization with O(1) lookup
- Improved path parameter handling
- Enhanced error messages
- Better performance monitoring

**Bug Fixes**:
- Fixed path parameter loss in nested routes
- Fixed potential stack overflow in route matching
- Fixed memory leaks in error handling

**Performance Improvements**:
- Peak throughput: 21,991 RPS (up from 19,776 RPS)
- Minimum latency: 551 μs (up from 352 μs)
- Router cache reduces route matching overhead by 50%+

## Release Schedule

### Development Branch

- **Branch**: `develop`
- **Status**: Active development
- **Stability**: May contain breaking changes

### Main Branch

- **Branch**: `main`
- **Status**: Stable release candidate
- **Stability**: Tested and stable

### Release Branch

- **Branch**: `release`
- **Status**: Production ready
- **Stability**: Fully tested and documented

### From 2.4 to 2.5

**Breaking Changes**:

1. **Constructor output-parameter style**
   - `uvhttp_server_new` and `uvhttp_router_new` now take an output parameter and
     return `uvhttp_error_t` instead of returning a pointer.

   **Migration**:
   ```c
   // Old (2.4.x)
   uvhttp_server_t* server = uvhttp_server_new(loop);
   uvhttp_router_t* router = uvhttp_router_new();
   server->router = router;

   // New (2.5.x)
   uvhttp_server_t* server = NULL;
   uvhttp_error_t r = uvhttp_server_new(loop, &server);
   if (r != UVHTTP_OK) { /* handle error */ }

   uvhttp_router_t* router = NULL;
   uvhttp_router_new(&router);
   uvhttp_server_set_router(server, router);  // use the setter, not direct struct access
   ```

2. **Request handler signature**
   - Handlers now receive both the request **and** the response, and return `int`.
     There is no `uvhttp_response_new()` — the framework creates the response.

   ```c
   // Old (2.4.x)
   void hello_handler(uvhttp_request_t* req) {
       uvhttp_response_t* res = uvhttp_response_new(req);
       uvhttp_response_set_body(res, "Hello");
       uvhttp_response_send(res);
   }

   // New (2.5.x)
   int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
       uvhttp_response_set_status(res, 200);
       uvhttp_response_set_body(res, "Hello");
       return uvhttp_response_send(res);
   }
   ```

3. **Memory-safety verification**
   - The full test suite is now verified clean under AddressSanitizer (no leaks,
     use-after-free, or overflows) and UndefinedBehaviorSanitizer. See the
     [Changelog](./CHANGELOG.md) for the complete list of fixes.

## Release Process

1. Development on `develop` branch
2. Merge to `main` when stable
3. Create release branch for version
4. Tag release
5. Deploy to production

## Support Policy

### LTS (Long Term Support)

- **Duration**: 6 months
- **Updates**: Security fixes only
- **Current LTS**: 2.5.x

### Stable

- **Duration**: 3 months
- **Updates**: Bug fixes and security fixes
- **Current Stable**: 2.5.x

### Development

- **Duration**: Until next stable release
- **Updates**: All changes including breaking changes
- **Current Development**: 2.6.x (develop branch)

## Getting Help

- **Documentation**: [Full Documentation](/)
- **Issues**: [GitHub Issues](https://github.com/adam-ikari/uvhttp/issues)
- **Discussions**: [GitHub Discussions](https://github.com/adam-ikari/uvhttp/discussions)

## Changelog

For detailed changelog, see [CHANGELOG.md](../CHANGELOG.md)