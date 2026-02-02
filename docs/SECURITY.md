# Security Policy

## Overview

This document describes the security policy for the UVHTTP project, including dependency management, vulnerability response procedures, and security best practices.

## Dependency Management

### Current Dependencies

| Dependency | Version | Purpose | Update Policy |
|------------|---------|---------|---------------|
| libuv | v1.51.0 | Event loop | Regular updates |
| mbedtls | v3.6.0 | TLS/SSL | Security updates priority |
| mimalloc | v3.1.5 | Memory allocator | Regular updates |
| cjson | v1.7.15 | JSON parsing | Regular updates |
| llhttp | v9.2.1 | HTTP parsing | Regular updates |
| uthash | v1.9.8 | Hash table | Regular updates |
| xxhash | v0.7.4 | Fast hashing | Regular updates |
| googletest | release-1.12.1 | Testing framework | As needed |

### Dependency Update Policy

#### 1. Security Updates (High Priority)
- **Trigger**: CVE vulnerability or serious security issue discovered
- **Response Time**: 7 days for assessment, 14 days for fix
- **Process**:
  1. Assess vulnerability impact scope
  2. Check upstream fix version
  3. Update dependency version
  4. Run full test suite
  5. Release security patch version

#### 2. Feature Updates (Medium Priority)
- **Trigger**: New features, performance improvements, API changes
- **Response Time**: Quarterly assessment
- **Process**:
  1. Evaluate new feature value
  2. Check API compatibility
  3. Update dependency version
  4. Update related documentation
  5. Release minor version

#### 3. Maintenance Updates (Low Priority)
- **Trigger**: Dependency version outdated (> 1 year)
- **Response Time**: Semi-annual assessment
- **Process**:
  1. Check compatibility
  2. Update dependency version
  3. Run tests
  4. Release patch version

### Dependency Version Pinning

All dependency versions are pinned in `.gitmodules` to ensure reproducible builds.

**Advantages**:
- Reproducible builds
- Avoid unexpected breaking changes
- Easier issue tracking

**Disadvantages**:
- Manual dependency updates required
- May miss security updates

**Mitigation**:
- Regular security scanning
- Subscribe to security advisories
- Establish automated checks

## Security Audits

### Regular Audit Schedule

| Audit Type | Frequency | Owner |
|------------|-----------|-------|
| Dependency vulnerability scan | Weekly | Automated |
| Code security review | Monthly | Security team |
| Penetration testing | Quarterly | Third-party |
| Architecture security review | Semi-annually | Security team |

### Automated Security Scanning

Use the following tools for automated security scanning:

1. **Dependency Scanning**
   ```bash
   # Use GitHub Dependabot
   # Configuration file: .github/dependabot.yml
   ```

2. **Static Code Analysis**
   ```bash
   # Use cppcheck
   cppcheck --enable=all src/
   ```

3. **Memory Safety Checks**
   ```bash
   # Use Valgrind
   valgrind --leak-check=full --show-leak-kinds=all ./uvhttp_server
   ```

4. **Address Sanitizer**
   ```bash
   # Use ASan for memory safety
   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
   ```

## Security Best Practices

### Input Validation

UVHTTP implements comprehensive input validation:

1. **URL Validation**
   - Maximum URL length: 2048 bytes
   - Path traversal protection
   - URL encoding validation

2. **Header Validation**
   - Maximum header count: 64
   - Maximum header name length: 256 bytes
   - Maximum header value length: 4096 bytes

3. **Body Size Limits**
   - Maximum body size: 10MB (configurable)
   - Chunked transfer encoding support

### Buffer Overflow Protection

All string operations use safe functions:

```c
// Safe string copy
if (uvhttp_safe_strcpy(dest, sizeof(dest), src) != 0) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

// Safe string length
size_t len = strlen(src);
if (len >= sizeof(dest)) {
    len = sizeof(dest) - 1;
}
strncpy(dest, src, len);
dest[len] = '\0';
```

### TLS Configuration

Recommended TLS configuration:

```c
// Enable TLS 1.3 only
mbedtls_ssl_conf_min_tls_version(&conf, MBEDTLS_SSL_TLS_1_3);

// Enable certificate verification
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);

// Set secure cipher suites
const int ciphers[] = {
    MBEDTLS_TLS_AES_256_GCM_SHA384,
    MBEDTLS_TLS_CHACHA20_POLY1305_SHA256,
    0
};
mbedtls_ssl_conf_ciphersuites(&conf, ciphers);
```

### DoS Protection

UVHTTP implements multiple DoS protection mechanisms:

1. **Rate Limiting**
   - Token bucket algorithm
   - Configurable limits per IP
   - Whitelist support

2. **Connection Limits**
   - Maximum connections: 2048 (configurable)
   - Connection timeout: 60 seconds
   - Request timeout: 30 seconds

3. **Resource Limits**
   - Maximum body size: 10MB
   - Maximum header size: 8KB
   - Maximum concurrent requests per connection: 100

## Vulnerability Reporting

### Reporting Process

If you discover a security vulnerability, please report it responsibly:

1. **Do not create a public issue**
2. **Send email to**: security@uvhttp.org
3. **Include**: Vulnerability description, reproduction steps, affected versions
4. **Response Time**: We will respond within 48 hours

### Vulnerability Handling Process

1. **Acknowledgment** (within 48 hours)
   - Confirm receipt of report
   - Assign severity level
   - Estimate fix timeline

2. **Assessment** (within 7 days)
   - Reproduce vulnerability
   - Assess impact
   - Develop fix

3. **Fix Development** (within 14 days)
   - Implement fix
   - Write tests
   - Review code

4. **Release** (within 21 days)
   - Prepare security advisory
   - Release patch version
   - Coordinate disclosure

### Severity Levels

| Severity | Description | Response Time |
|----------|-------------|---------------|
| Critical | Remote code execution | 48 hours |
| High | Data leakage or DoS | 7 days |
| Medium | Information disclosure | 14 days |
| Low | Minor security issue | 30 days |

## Security Features

### Memory Safety

- **Zero compilation warnings**: All code compiles with `-Werror`
- **Memory allocator**: mimalloc for improved memory safety
- **Buffer overflow protection**: All string operations validated
- **Memory leak detection**: Regular Valgrind testing

### Input Validation

- **URL validation**: Length limits, path traversal protection
- **Header validation**: Size limits, format validation
- **Body validation**: Size limits, encoding validation
- **Parameter validation**: Type checking, range validation

### Secure Defaults

- **TLS 1.3**: Enabled by default when TLS is used
- **Certificate verification**: Required by default
- **Secure cipher suites**: Pre-configured
- **Rate limiting**: Enabled by default

## Security Checklist

Before deploying to production, ensure:

- [ ] All dependencies are up to date
- [ ] No known vulnerabilities in dependencies
- [ ] TLS is properly configured
- [ ] Rate limiting is enabled
- [ ] Input validation is comprehensive
- [ ] Error messages don't leak sensitive information
- [ ] Logging doesn't expose sensitive data
- [ ] File permissions are correct
- [ ] Firewall rules are configured
- [ ] Monitoring and alerting are set up

## Security Resources

- **Security Advisories**: https://github.com/adam-ikari/uvhttp/security/advisories
- **CVE Database**: https://cve.mitre.org/
- **OWASP Top 10**: https://owasp.org/www-project-top-ten/
- **Security Best Practices**: https://wiki.sei.cmu.edu/confluence/display/seccode/Top+10+CERT+C+Coding+Rules

## Contact

For security-related questions or to report vulnerabilities:
- **Email**: security@uvhttp.org
- **GitHub Security**: https://github.com/adam-ikari/uvhttp/security
- **PGP Key**: Available on request

---

**Last Updated**: 2026-02-02  
**Version**: 1.0  
**Maintainer**: UVHTTP Security Team