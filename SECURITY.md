# Security Policy

## Supported Versions

| Version | Supported          |
|---------|--------------------|
| 2.5.x   | :white_check_mark: |
| < 2.5   | :x:                |

## Reporting a Vulnerability

UVHTTP takes memory safety and security seriously. We use AddressSanitizer and UndefinedBehaviorSanitizer in CI to verify every commit, and we encourage responsible disclosure.

To report a security vulnerability:

1. **Do not open a public GitHub issue.** Instead, email the maintainer directly or open a [GitHub Security Advisory](https://github.com/adam-ikari/uvhttp/security/advisories/new).

2. Include the following in your report:
   - Description of the vulnerability
   - Steps to reproduce
   - Affected versions
   - Potential impact
   - Suggested fix (if any)

3. You should receive a response within 48 hours. If not, follow up via the same channel.

## Disclosure Policy

- Vulnerabilities are disclosed publicly only after a fix has been released and users have had reasonable time to update.
- Contributors who report valid vulnerabilities will be acknowledged in the release notes (unless they request anonymity).

## Memory Safety

UVHTTP is committed to memory safety. The project maintains a CI gate (`make verify-memory-safety`) that runs the full test suite under both AddressSanitizer and UndefinedBehaviorSanitizer. Any regression is blocked before merge.

For details, see [MEMORY_SAFETY.md](./MEMORY_SAFETY.md).

## Security-Related Configuration

See the [TLS Configuration Guide](./guide/TLS.md) for best practices on:
- Certificate management
- Cipher suite selection
- TLS version configuration
- Client authentication (mTLS)