# uvhttp Dependency Management Guide

uvhttp builds on a small set of third-party libraries. This page records the version each one is pinned to and how we upgrade them.

## Core Dependencies

### 1. libuv

libuv v1.51.0 supplies the asynchronous I/O event loop. It is required, licensed under MIT, and the version is locked.

### 2. mbedtls

mbedtls v3.6.0 handles TLS/SSL. It is required, licensed under Apache 2.0, and locked. mbedtls is the only TLS implementation in the project: HTTPS and mTLS both go through it.

### 3. llhttp

llhttp parses HTTP. The project tracks the latest stable release (9.3.1) as a Git submodule, under the MIT license. It has one build step: run `npm install` and `make build/libllhttp.a` to generate the C source files.

### 4. xxhash

xxhash v0.7.4 computes hashes. It is required, licensed under BSD 2-Clause, and locked.

### 5. uthash

uthash v1.9.8 provides the hash table. It is header-only, required, licensed under BSD Revised, and locked.

### WebSocket Implementation

The WebSocket layer is implemented natively in uvhttp_websocket_native.c rather than on a third-party library. That keeps the dependency list short and leaves the code under our control.

## Optional Dependencies

### 7. cjson

cJSON v1.7.15 handles JSON. It is optional, licensed under MIT, and locked.

### 8. mimalloc

mimalloc v3.1.5 is an alternative memory allocator, enabled with the BUILD_WITH_MIMALLOC option. It is licensed under MIT and locked. The code supports it, but the default build does not enable it.

## Test Dependencies

### 9. googletest

googletest release-1.12.1 is the unit test framework. It is licensed under BSD 3-Clause and locked; the project upgraded to it from 1.8.0.

## Removed Dependencies

### cmocka

cmocka was removed on 2026-01-05. The project uses googletest, and cmocka was never actually used.

## Dependency Upgrade Strategy

### Upgrade Principles

1. Upgrade as soon as a security advisory lands.
2. Test compatibility before merging an upgrade.
3. Lock every version so nothing moves by accident.
4. Upgrade test dependencies before core dependencies.

### Upgrade Process

1. Read the changelog and security advisories for the new version.
2. Do the upgrade on a development branch.
3. Run the full test suite.
4. Re-measure the performance metrics.
5. Update this document.
6. Open a PR and get it reviewed.

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

TLS comes from mbedtls. The build turns on `-D_FORTIFY_SOURCE=2`, `-fstack-protector-strong`, `-Wformat-security`, `-fno-omit-frame-pointer`, `-Werror=implicit-function-declaration`, `-Werror=format-security`, and `-Werror=return-type`. The linker adds `-Wl,-z,relro` for read-only relocations and `-Wl,-z,now` for immediate binding. googletest sits on a current release, and every dependency has a fixed version.

### Security Audit Recommendations

1. Check the security advisories for each dependency.
2. Run dependency scanners such as `npm audit` and `snyk`.
3. Audit dependencies once a quarter.
4. Watch the CVE database for relevant vulnerabilities.
5. Keep mbedtls on the latest stable release.

## Dependency Update Changelog

### 2026-01-05

googletest moved from release-1.8.0 to release-1.12.1. cmocka was dropped because nothing used it. TLS was unified on mbedtls, and OpenSSL was removed. All dependencies gained version locks, and the secure compile options were added.

## Contact

For dependency-related questions, contact the project maintainers or submit an issue.
