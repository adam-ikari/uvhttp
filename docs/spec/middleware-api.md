# Middleware Spec

## Overview

The Middleware module provides a compile-time, zero-overhead middleware system
using C preprocessor macros. Middleware are chained at compile time, with no
runtime registration or dispatch overhead.

## Interfaces

### UVHTTP_EXECUTE_MIDDLEWARE
- **Signature**: `UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw1, mw2, ...)`
- **Purpose**: Execute middleware chain inline
- **Preconditions**: `req` and `resp` must be valid pointers. Each middleware must be a function with signature `int (*)(uvhttp_request_t*, uvhttp_response_t*)`.
- **Postconditions**: Middleware are executed in order. If any returns non-zero (STOP), remaining middleware are skipped. The local variable `mw_ctx` is available for context sharing.
- **Thread safety**: Not thread-safe (same as the server).

### UVHTTP_DEFINE_MIDDLEWARE_CHAIN
- **Signature**: `UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, mw1, mw2, ...)`
- **Purpose**: Define a reusable middleware chain as a static array
- **Preconditions**: Name must be a valid C identifier. Each middleware must be a valid function pointer.
- **Postconditions**: A static array named `uvhttp_mw_chain_##name` is created.
- **Thread safety**: Thread-safe for reads.

### UVHTTP_EXECUTE_MIDDLEWARE_CHAIN
- **Signature**: `UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, chain)`
- **Purpose**: Execute a pre-defined middleware chain
- **Preconditions**: `chain` must be a name previously defined with `UVHTTP_DEFINE_MIDDLEWARE_CHAIN`.
- **Postconditions**: Same as `UVHTTP_EXECUTE_MIDDLEWARE`.
- **Thread safety**: Not thread-safe.

### UVHTTP_DEFINE_MIDDLEWARE_HANDLER
- **Signature**: `UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)`
- **Purpose**: Wrap a route handler as a middleware (returns 0 = CONTINUE)
- **Preconditions**: `handler` must be a function with signature `int (*)(uvhttp_request_t*, uvhttp_response_t*)`.
- **Postconditions**: Returns UVHTTP_MIDDLEWARE_CONTINUE (0) if handler returns 0, otherwise UVHTTP_MIDDLEWARE_STOP (1).

## Behavior Rules

1. **Compile-time only**: All middleware chains are resolved at compile time. No runtime registration or lookup.

2. **Short-circuit semantics**: If any middleware returns UVHTTP_MIDDLEWARE_STOP (non-zero), the chain halts immediately. Subsequent middleware are not executed.

3. **Context sharing**: All middleware in a chain receive the same `req` and `resp` pointers. A shared context variable `mw_ctx` is available as a local `void*` that can be set by one middleware and read by subsequent ones.

4. **Handler wrapping**: Route handlers can be wrapped as middleware via `UVHTTP_DEFINE_MIDDLEWARE_HANDLER`. A handler that returns 0 is treated as CONTINUE; non-zero is STOP.

## Performance Requirements

- Zero runtime overhead compared to hand-written inline code
- No dynamic memory allocation
- No function pointer tables (chains are static arrays)

## Test Requirements

- Middleware execution order
- STOP semantics (short-circuit)
- Context sharing between middleware
- Chain definition and execution
- Handler wrapping
- Multiple chains in same scope
- NULL handler in chain (should be skipped)
- Nested middleware (middleware that calls another chain)