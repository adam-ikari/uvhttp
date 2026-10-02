# UVHTTP Roadmap

## Vision

A lightweight, production-grade **HTTP/1.1** server library for C — with no hidden layer between your application and libuv.

## Scope Boundary

What this library is deliberately **not** responsible for. Each was evaluated and excluded; the reasoning for the protocol choices is recorded in [PHILOSOPHY](../PHILOSOPHY.md).

| Concern | Why it is out of scope |
|---|---|
| HTTP/2 | Multiplexing is incompatible with libuv's transparent per-connection event model |
| HTTP/3 (QUIC) | Requires a different transport layer; outside the scope of a lightweight C library |
| IPv6 **listening** | Gateway / network-stack concern. Client-side IPv6 address extraction *is* supported |
| YAML/JSON config files | The framework or application embedding this library owns its own configuration format |
| Per-user / per-IP rate limiting | Gateway / reverse-proxy concern. A global token bucket with whitelist *is* provided |
| Observability (metrics, tracing, dashboards) | Prometheus / OpenTelemetry territory |
| Auth, DDoS protection, certificate management | Gateway / security-layer concern |
| Plugin system, microservices, cloud-native, serverless | Infrastructure. Compile-time feature selection (27 CMake options) covers capability needs |
| Hot reload | Process/supervisor concern |

## Current Status (v2.9.1)

### Delivered

- **HTTP/1.1** — ~85K RPS on CI runners (paired-gate measured)
- **WebSocket** — full-duplex, RFC 6455, Ping/Pong with heartbeat
- **TLS 1.2/1.3** via mbedtls, with session cache (2048 entries / 24h)
- **Static files** — `sendfile` zero-copy with chunked fallback; path resolution fuzz-verified
- **gzip** compression with LRU cache
- **Rate limiting** — global token bucket + whitelist
- **Middleware** — compile-time, zero runtime overhead
- **32-bit embedded** support
- **27 compile-time options** for capability trimming

### Quality Infrastructure

- 102 unit tests, all green
- ASan gate on every PR; UBSan, stress, and coverage runs nightly
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

Absolute values drift roughly 40% across runs on shared runners, so **only paired ratios are meaningful**. The gate fails when the median ratio drops below 90% *and* most individual pairs also fall below 90% — absolute RPS thresholds would gate on runner luck rather than on code.
