# UVHTTP Specification Index

This directory contains the formal specifications for the UVHTTP project,
following Spec Driven Development (SDD) principles. Each spec defines the
behavior contract, interfaces, and quality criteria for a module.

## Spec Documents

### Core API Specifications
- [Server API Spec](server-api.md) — `uvhttp_server_t` lifecycle, listen, stop
- [Router API Spec](router-api.md) — `uvhttp_router_t` route matching, cache
- [Connection API Spec](connection-api.md) — `uvhttp_connection_t` lifecycle
- [Request API Spec](request-api.md) — `uvhttp_request_t` parsing, headers, body
- [Response API Spec](response-api.md) — `uvhttp_response_t` building, sending
- [WebSocket API Spec](websocket-api.md) — WebSocket handshake, frames, close
- [Static File API Spec](static-api.md) — `uvhttp_static_t` file serving, cache
- [TLS API Spec](tls-api.md) — `uvhttp_tls_t` context, handshake, config
- [Config API Spec](config-api.md) — `uvhttp_config_t` options, validation
- [Error API Spec](error-api.md) — `uvhttp_error_t` codes, messages, handling

### Quality Specifications
- [Memory Safety Spec](memory-safety.md) — ASan/UBSan gates, leak policy
- [Test Coverage Spec](test-coverage.md) — Coverage targets, test categories
- [Performance Spec](performance.md) — Throughput, latency, resource limits
- [Build System Spec](build-system.md) — CMake configuration, C standard, flags

### Internal Specifications
- [Allocator Spec](allocator.md) — `uvhttp_allocator_t` interface, backends
- [Protocol Upgrade Spec](protocol-upgrade.md) — HTTP upgrade mechanism

## Spec Format

Each spec document follows this template:

```markdown
# Module Name Spec

## Overview
Brief description of the module's purpose and responsibilities.

## Interfaces
### Function Name
- **Signature**: `return_type function_name(params)`
- **Purpose**: What it does
- **Preconditions**: What must be true before calling
- **Postconditions**: What is true after calling
- **Error conditions**: When it returns errors
- **Thread safety**: Thread-safe or not

## Behavior Rules
Numbered rules that define the module's behavior contract.

## Performance Requirements
Specific throughput, latency, or memory targets.

## Test Requirements
What test scenarios must exist.
```

## Compliance

Each spec is verified by:
1. **Static analysis**: Code review against spec rules
2. **Unit tests**: Test cases covering each spec requirement
3. **Integration tests**: End-to-end scenarios
4. **CI gates**: Automated checks on every PR