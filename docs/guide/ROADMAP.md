# UVHTTP Roadmap

## Vision

A lightweight, production-grade **HTTP/1.1** server library for C. Your application talks to libuv directly.

## Scope Boundary

What the library deliberately leaves out. Each item was evaluated and excluded; the reasoning for the protocol choices is recorded in [PHILOSOPHY](../PHILOSOPHY.md).

| Concern | Why it is out of scope |
|---|---|
| HTTP/2 | Multiplexing is incompatible with libuv's transparent per-connection event model |
| HTTP/3 (QUIC) | Requires a different transport layer; outside the scope of a lightweight C library |
| IPv6 **listening** | Gateway / network-stack concern. Client-side IPv6 address extraction *is* supported |
| YAML/JSON config files | The framework or application embedding this library owns its own configuration format |
| Per-user / per-IP rate limiting | Gateway / reverse-proxy concern. The global token bucket was removed in v3.0.0; the pre-dispatch request filter is where you implement it |
| Observability (metrics, tracing, dashboards) | Prometheus / OpenTelemetry territory |
| Auth, DDoS protection, certificate management | Gateway / security-layer concern |
| Plugin system, microservices, cloud-native, serverless | Infrastructure. Compile-time feature selection (27 CMake options) covers capability needs |
| Hot reload | Process/supervisor concern |

## Current Status (v3.0.0)

### Delivered

- HTTP/1.1 handles ~85K RPS on CI runners (paired-gate measured).
- WebSocket is full-duplex over RFC 6455, with Ping/Pong heartbeat.
- TLS 1.2/1.3 runs on mbedtls, with a session cache of 2048 entries for 24h.
- Static files use `sendfile` zero-copy with a chunked fallback; path resolution is fuzz-verified.
- gzip compression with an LRU cache.
- Rate limiting runs in the pre-dispatch request filter; the library ships no limiter.
- Middleware is compile-time and adds no runtime overhead.
- 32-bit embedded support.
- 27 compile-time options trim capabilities.

### Quality Infrastructure

- 119 unit tests, all green
- ASan runs in the pre-release gate; UBSan, stress, and coverage run nightly
- 4 libFuzzer harnesses (router / request / websocket / static-path)
- Same-runner paired performance gate: head vs base, 10% threshold, fail-closed
- cppcheck static analysis, zero warnings
- CI `GITHUB_TOKEN` scoped to `contents: read`

## Open Work

Genuine gaps, ordered by value.

### Performance

- [ ] Connection pooling optimization
- [ ] Memory usage reduction per request
- [ ] CPU efficiency on the hot path

### Developer Experience

- [ ] Enhanced logging framework (structured levels, pluggable sinks)
- [ ] API reference completeness pass

### Documentation

- [ ] Architecture diagrams
- [ ] Performance tuning guide

### Platform — no schedule, on demand

- [ ] macOS
- [ ] FreeBSD

## Technology Stack

| Layer | Choice |
|---|---|
| Core | C99, libuv 1.52, llhttp |
| TLS | mbedtls |
| Memory | mimalloc (optional) |
| Hashing | xxHash |
| JSON | cJSON (optional) |
| Testing | Google Test, libFuzzer |

## Measured Performance

From the paired gate (same runner, head vs base, 10 alternating rounds):

| Endpoint | RPS | Note |
|---|---|---|
| `/` | ~85K | In-memory response |
| `/json` | ~85K | JSON serialization |
| `/large` | ~9.6K | 100KB body |

Absolute values drift roughly 40% across runs on shared runners, so **only paired ratios are meaningful**. The gate fails when the median ratio drops below 90% *and* most individual pairs also fall below 90%. Absolute RPS thresholds would gate on runner luck rather than on code.
