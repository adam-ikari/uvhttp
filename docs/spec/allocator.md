# Allocator Spec

## Overview

One allocation interface covers every backend, and the build selects the
backend at compile time. All modules allocate and free through this interface.

## Interfaces

### uvhttp_alloc
- **Signature**: `void* uvhttp_alloc(size_t size)`
- **Purpose**: Allocate memory
- **Preconditions**: `size` > 0
- **Returns**: Pointer to allocated memory, or NULL on failure.
- **Thread safety**: Depends on backend (system allocator is thread-safe).

### uvhttp_free
- **Signature**: `void uvhttp_free(void* ptr)`
- **Purpose**: Free allocated memory
- **Preconditions**: `ptr` must be from `uvhttp_alloc`, `uvhttp_calloc`, or `uvhttp_realloc`. Can be NULL (no-op).
- **Thread safety**: Depends on backend.

### uvhttp_realloc
- **Signature**: `void* uvhttp_realloc(void* ptr, size_t size)`
- **Purpose**: Reallocate memory
- **Preconditions**: `ptr` must be from a previous allocation. Can be NULL (equivalent to alloc).
- **Returns**: Pointer to reallocated memory, or NULL on failure.
- **Thread safety**: Depends on backend.

### uvhttp_calloc
- **Signature**: `void* uvhttp_calloc(size_t nmemb, size_t size)`
- **Purpose**: Allocate and zero-initialize memory
- **Preconditions**: `nmemb * size` > 0
- **Returns**: Pointer to zero-initialized memory, or NULL on failure.
- **Thread safety**: Depends on backend.

### uvhttp_allocator_name
- **Signature**: `const char* uvhttp_allocator_name(void)`
- **Purpose**: Get the name of the current allocator backend
- **Returns**: "system", "mimalloc", or "custom".
- **Thread safety**: Thread-safe.

## Backends

| Type | Value | Description |
|------|-------|-------------|
| System | 0 | Standard `malloc`/`free` |
| mimalloc | 1 | Microsoft mimalloc |
| Custom | 2 | User-provided allocator |

## Behavior Rules

1. **Rule 1**: Never mix allocators. Memory allocated with `uvhttp_alloc` must be freed with `uvhttp_free`.

2. **Rule 2**: Check every allocation result. NULL means out of memory.

3. **Rule 3**: Pair every allocation with a free. The code must not leak.

4. **Rule 4**: `uvhttp_free` accepts NULL and does nothing.

## Test Requirements

The tests cover basic allocation and free, zero-size allocation, large
allocation, and reallocation. They also check calloc zero-initialization, NULL
free (no-op), allocator name retrieval, and a leak-free run under ASan.
