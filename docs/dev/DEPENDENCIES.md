# uvhttp Dependency Management Guide

This document records the version information and compatibility notes for all third-party dependency libraries used by the uvhttp project.

## Core Dependencies

### 1. libuv
- **Version**: v1.51.0
- **Purpose**: Asynchronous I/O event loop core library
- **Type**: Required dependency
- **License**: MIT
- **Status**: Locked

### 2. mbedtls
- **Version**: v3.6.0
- **Purpose**: TLS/SSL encryption support
- **Type**: Required dependency
- **License**: Apache 2.0
- **Status**: Locked
- **Notes**: The project uses mbedtls as its sole TLS library implementation. All TLS-related functionality (including HTTPS, mTLS) is implemented through mbedtls.

### 3. llhttp
- **Version**: Latest stable (9.3.1)
- **Purpose**: HTTP protocol parsing
- **Type**: Required dependency (Git submodule)
- **License**: MIT
- **Status**: In use
- **Build method**: Requires running `npm install` and `make build/libllhttp.a` first to generate C source files

### 4. xxhash
- **Version**: v0.7.4
- **Purpose**: Hash computation
- **Type**: Required dependency
- **License**: BSD 2-Clause
- **Status**: Locked

### 5. uthash
- **Version**: v1.9.8
- **Purpose**: Hash table data structure
- **Type**: Required dependency (header-only library)
- **License**: BSD Revised
- **Status**: Locked

### WebSocket Implementation
- **Implementation method**: Native implementation (uvhttp_websocket_native.c)
- **Notes**: Does not depend on any third-party WebSocket library; fully self-implemented
- **Advantages**: Lighter, more controllable, no extra dependencies

## Optional Dependencies

### 7. cjson
- **Version**: v1.7.15
- **Purpose**: JSON data processing
- **Type**: Optional dependency
- **License**: MIT
- **Status**: Locked

### 8. mimalloc
- **Version**: v3.1.5
- **Purpose**: Memory allocator
- **Type**: Optional dependency (enabled via the BUILD_WITH_MIMALLOC option)
- **License**: MIT
- **Status**: Locked; supported in code but not enabled by default

## Test Dependencies

### 9. googletest
- **Version**: release-1.12.1
- **Purpose**: Unit testing framework
- **Type**: Test dependency
- **License**: BSD 3-Clause
- **Status**: Locked (upgraded from 1.8.0)

## Removed Dependencies

### cmocka
- **Status**: Removed
- **Reason**: The project actually uses googletest as its testing framework; cmocka was not used
- **Removal date**: 2026-01-05

## Dependency Upgrade Strategy

### Upgrade Principles
1. **Security first**: Upgrade immediately when security vulnerabilities are discovered
2. **Compatibility testing**: Full compatibility testing must be performed before upgrading
3. **Version locking**: All dependencies use fixed versions to avoid unexpected updates
4. **Progressive upgrades**: Prioritize upgrading test dependencies first, then core dependencies

### Upgrade Process
1. Check the new version's changelog and security advisories
2. Perform the upgrade on a development branch
3. Run the full test suite
4. Verify performance metrics
5. Update this document
6. Submit a PR and undergo code review

### Version Compatibility Matrix

| Dependency | Current Version | Minimum Compatible Version | Recommended Version |
|------|---------|-------------|---------|
| libuv | v1.51.0 | v1.44.0 | v1.51.0 |
| mbedtls | v3.6.0 | v3.0.0 | v3.6.0 |
| llhttp | latest | v6.0.0 | latest |
| xxhash | v0.7.4 | v0.7.0 | v0.7.4 |
| uthash | v1.9.8 | v1.9.0 | v1.9.8 |
| libwebsockets | v4.5.0 | v4.3.0 | v4.5.0 |
| cjson | v1.7.15 | v1.7.0 | v1.7.15 |
| mimalloc | v3.1.5 | v2.0.0 | v3.1.5 |
| googletest | release-1.12.1 | release-1.10.0 | release-1.12.1 |

**Note**: The project uses mbedtls as its TLS library implementation. The libwebsockets precompiled library uses OpenSSL, but the project's own TLS functionality is entirely based on mbedtls.

## Compile Options

### Memory Allocator Selection

```bash
# System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc allocator
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### Logging System

```bash
# Enable the logging system (default)
cmake -DUVHTTP_FEATURE_LOGGING=ON ..

# Disable the logging system (zero overhead)
cmake -DUVHTTP_FEATURE_LOGGING=OFF ..
```

## Build Options

### Standard Build
```bash
make build
```

### Debug Build
```bash
make build
```

### Coverage Build
```bash
make build
```

### Using mimalloc
```bash
make build
```

### Combined Options
```bash
make build
```

## Security Considerations

### Current Security Measures
- Use mbedtls as the TLS library (lightweight, secure)
- Enable compile-time security flags:
  - `-D_FORTIFY_SOURCE=2`
  - `-fstack-protector-strong`
  - `-Wformat-security`
  - `-fno-omit-frame-pointer`
  - `-Werror=implicit-function-declaration`
  - `-Werror=format-security`
  - `-Werror=return-type`
- Enable linker security flags:
  - `-Wl,-z,relro` (read-only relocations)
  - `-Wl,-z,now` (immediate binding)
- Upgrade googletest to the latest stable version (to fix known vulnerabilities)
- All dependencies use fixed versions

### Security Audit Recommendations
1. Regularly check security advisories for dependency libraries
2. Use dependency scanning tools (such as `npm audit`, `snyk`, etc.)
3. Perform a dependency security audit quarterly
4. Monitor the CVE database for relevant vulnerabilities
5. Regularly update mbedtls to the latest stable version

## Dependency Update Changelog

### 2026-01-05
- Upgraded googletest: release-1.8.0 → release-1.12.1
- Removed cmocka (unused testing framework)
- Unified the TLS library to mbedtls (removed OpenSSL)
- Added version locking for all dependencies
- Added secure compile options

## Contact

For dependency-related questions, contact the project maintainers or submit an issue.
