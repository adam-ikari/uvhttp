# Versions

This document provides information about UVHTTP versions and their compatibility.

## Current Version

**Version**: 2.2.0  
**Release Date**: 2025-01-30  
**Status**: Stable

## Version History

### 2.2.0 (2025-01-30)

**Major Changes**:
- Split CI/CD into separate 32-bit and 64-bit workflows
- Add i18n support for documentation (English and Chinese)
- Fix 32-bit build compatibility issues
- Update validation functions for better security

**Improvements**:
- Performance optimization: peak throughput up to 23,226 RPS
- Better error handling and reporting
- Improved documentation

**Bug Fixes**:
- Fix shift overflow in 32-bit WebSocket implementation
- Fix validation functions for 32-bit compatibility
- Fix CI/CD workflow issues

### 2.1.0 (2025-01-20)

**Major Changes**:
- Refactor to remove global variables
- Implement libuv data pointer pattern
- Add comprehensive test coverage

**New Features**:
- WebSocket support
- Static file serving
- Rate limiting
- Memory leak detection

**Performance**:
- Zero-copy optimization for large files
- LRU cache implementation
- Connection pooling

### 2.0.0 (2025-01-10)

**Major Changes**:
- Complete rewrite from scratch
- New API design
- Modular architecture

**Breaking Changes**:
- New API incompatible with 1.x
- All functions renamed to `uvhttp_module_action` format
- New error handling system

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
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;
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

**No Breaking Changes**:

Minor improvements and bug fixes. No code changes required.

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
- **Current LTS**: 2.2.x

### Stable

- **Duration**: 3 months
- **Updates**: Bug fixes and security fixes
- **Current Stable**: 2.2.x

### Development

- **Duration**: Until next stable release
- **Updates**: All changes including breaking changes
- **Current Development**: 2.3.x (develop branch)

## Getting Help

- **Documentation**: [Full Documentation](/)
- **Issues**: [GitHub Issues](https://github.com/adam-ikari/uvhttp/issues)
- **Discussions**: [GitHub Discussions](https://github.com/adam-ikari/uvhttp/discussions)

## Changelog

For detailed changelog, see [CHANGELOG.md](../CHANGELOG.md)