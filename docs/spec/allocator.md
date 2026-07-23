# Allocator Spec

## Overview

The Allocator module provides a unified memory allocation interface. The
allocation backend is selected at compile time.

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

2. **Rule 2**: Always check allocation results. NULL means out of memory.

3. **Rule 3**: Every allocation must have a matching free. No leaks.

4. **Rule 4**: NULL can be passed to `uvhttp_free` safely (no-op).

## Test Requirements

- Basic allocation and free
- Zero-size allocation
- Large allocation
- Reallocation
- Calloc zero-initialization
- NULL free (no-op)
- Allocator name retrieval
- No leaks under ASan