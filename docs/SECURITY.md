# Security Policy

## Supported Versions

| Version | Supported          |
|---------|--------------------|
| 2.5.x   | :white_check_mark: |
| < 2.5   | :x:                |

## Reporting a Vulnerability

UVHTTP runs AddressSanitizer and UndefinedBehaviorSanitizer in CI, and bugs that reach a release are treated as security bugs. Report them privately.

To report a security vulnerability:

1. **Do not open a public GitHub issue.** Email the maintainer directly, or open a [GitHub Security Advisory](https://github.com/adam-ikari/uvhttp/security/advisories/new).

2. Include in your report:
   - Description of the vulnerability
   - Steps to reproduce
   - Affected versions
   - Potential impact
   - Suggested fix (if any)

3. You should get a response within 48 hours. If you do not, follow up through the same channel.

## Disclosure Policy

- A vulnerability becomes public only after a fix ships and users have had time to update.
- Reporters of valid vulnerabilities are named in the release notes, unless they ask to stay anonymous.

## Memory Safety

Memory safety is a release condition, not an aspiration. The `make verify-memory-safety` target runs the full suite under AddressSanitizer and UndefinedBehaviorSanitizer, and the release gate blocks on a finding.

Details are in [MEMORY_SAFETY.md](./MEMORY_SAFETY.md).

## Security-Related Configuration

The [TLS documentation](./guide/build.md) covers:
- Certificate management
- Cipher suite selection
- TLS version configuration
- Client authentication (mTLS)
