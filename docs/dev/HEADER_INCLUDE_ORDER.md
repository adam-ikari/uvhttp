# Header Include Order Specification

This document defines the header include order specification for the UVHTTP project, to ensure code consistency and maintainability.

## Specification Overview

Header include order follows these principles:

1. **Relevance first**: related headers should be grouped together
2. **Local first**: project internal headers take precedence over system headers
3. **Group by type**: group by type (local, system, third-party)
4. **Alphabetical order**: sort alphabetically within the same group

## Include Order

### 1. Corresponding Header (for .c files)

Each .c file should first include its corresponding .h file (if present).

```c
#include "uvhttp_server.h"
```

### 2. UVHTTP Project Headers

All project headers prefixed with `uvhttp_`, sorted alphabetically.

```c
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"
```

### 3. Other Internal Project Headers

Other internal project headers (not prefixed with `uvhttp_`), sorted alphabetically.

```c
#include "uthash.h"
```

### 4. Standard Library Headers

C standard library headers, sorted alphabetically.

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

### 5. Third-Party Library Headers

Third-party library headers, sorted alphabetically.

```c
#include <uv.h>
```

### 6. Conditional Compilation Includes

Conditional compilation headers should be placed last, sorted alphabetically.

```c
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif

#ifdef UVHTTP_FEATURE_TLS
#include "uvhttp_tls.h"
#endif
```

## Complete Example

### .c File Example

```c
/*
 * UVHTTP server module
 *
 * Provides the core functionality of the HTTP server
 */

#include "uvhttp_server.h"

// UVHTTP project headers
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"

// Standard library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party library headers
#include <uv.h>

// Conditional compilation includes
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

### .h File Example

```c
#ifndef UVHTTP_SERVER_H
#    define UVHTTP_SERVER_H

// UVHTTP project headers
#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"

// Standard library headers
#include <assert.h>

// Third-party library headers
#include <uv.h>

// Other internal project headers
#include "uthash.h"

// UVHTTP project headers (dependencies)
#include "uvhttp_features.h"

// Conditional compilation includes
#    if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#    endif

#endif // UVHTTP_SERVER_H
```

## Automatic Formatting

Use `clang-format` to automatically format the include order:

```bash
# Check format
make format-check

# Fix format
make format-fix

# Format all files
make format-all
```

## Rule Explanations

### 1. Corresponding Header First

- Each .c file should first include its corresponding .h file
- This helps verify header self-containment

### 2. UVHTTP Project Header Grouping

- All headers prefixed with `uvhttp_` are grouped together
- Sorted alphabetically
- Easier to find and maintain

### 3. Other Internal Project Headers

- Project headers not prefixed with `uvhttp_`
- e.g. `uthash.h`
- Sorted alphabetically

### 4. Standard Library Headers

- C standard library headers (`<*.h>`)
- Sorted alphabetically
- Easier to identify dependencies

### 5. Third-Party Library Headers

- Third-party library headers (`<*>`)
- e.g. `<uv.h>`, `<mbedtls/*.h>`
- Sorted alphabetically

### 6. Conditional Compilation Includes

- All conditional compilation includes are placed last
- Sorted alphabetically
- Easier to read and maintain

## Common Mistakes

### Mistake 1: System Headers First

❌ Wrong:

```c
#include <stdio.h>
#include <stdlib.h>
#include "uvhttp_server.h"
#include "uvhttp_config.h"
```

✅ Correct:

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#include <stdio.h>
#include <stdlib.h>
```

### Mistake 2: Not Alphabetical

❌ Wrong:

```c
#include "uvhttp_utils.h"
#include "uvhttp_config.h"
#include "uvhttp_server.h"
```

✅ Correct:

```c
#include "uvhttp_config.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"
```

### Mistake 3: Conditional Compilation Includes Mixed In The Middle

❌ Wrong:

```c
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
#include "uvhttp_config.h"
```

✅ Correct:

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

## Tool Support

### clang-format

The project uses `clang-format` to automatically format code, including header include order.

Configuration file: `.clang-format`

### Makefile Commands

```bash
# Check format
make format-check

# Fix format
make format-fix

# Format all files
make format-all
```

## CI/CD Integration

Format checks can be added to the CI/CD workflow:

```yaml
- name: Check code format
  run: |
    make format-check
```

## Reference Resources

- [Google C++ Style Guide - Names and Order of Includes](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes)
- [clang-format - IncludeCategories](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#includecategories)

---

**Last Updated**: 2026-01-29
**Maintainer**: UVHTTP Development Team
**License**: MIT License
