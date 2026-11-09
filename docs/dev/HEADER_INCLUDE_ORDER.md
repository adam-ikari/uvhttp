# Header Include Order Specification

## Specification Overview

Includes follow four rules:

1. Related headers sit together.
2. Local headers come before system headers.
3. Headers are grouped by type: local, system, third-party.
4. Each group is sorted alphabetically.

## Include Order

### 1. Corresponding Header (for .c files)

A .c file includes its own header first, if it has one.

```c
#include "uvhttp_server.h"
```

### 2. UVHTTP Project Headers

Project headers carry the `uvhttp_` prefix. Sort them alphabetically.

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

Internal headers without the `uvhttp_` prefix come next, sorted alphabetically.

```c
#include "uthash.h"
```

### 4. Standard Library Headers

The C standard library headers follow, sorted alphabetically.

```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

### 5. Third-Party Library Headers

Third-party library headers come next, sorted alphabetically.

```c
#include <uv.h>
```

### 6. Conditional Compilation Includes

Conditional includes go last, sorted alphabetically.

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

`clang-format` sorts the includes for you:

```bash
# Format files in place
clang-format -i src/uvhttp_server.c

# Check without writing
clang-format --dry-run --Werror src/uvhttp_server.c
```

`just fmt` formats every C source and header under `src/`, `include/`, and `test/`; `just check-style` runs the same check without writing.

## Rule Explanations

### 1. Corresponding Header First

Each .c file includes its own header first. That is what proves the header is self-contained.

### 2. UVHTTP Project Header Grouping

Headers with the `uvhttp_` prefix form one group, sorted alphabetically. The grouping makes them easy to find and maintain.

### 3. Other Internal Project Headers

Internal headers without the `uvhttp_` prefix get their own alphabetized group, such as `uthash.h`.

### 4. Standard Library Headers

C standard library headers (`<*.h>`) are sorted alphabetically, which makes the dependencies of a file obvious.

### 5. Third-Party Library Headers

Third-party headers (`<*>`) come next, for example `<uv.h>` and `<mbedtls/*.h>`. Sort them alphabetically.

### 6. Conditional Compilation Includes

Conditional includes go last, sorted alphabetically. Keeping them out of the middle makes the unconditional list easy to scan.

## Common Mistakes

### Mistake 1: System Headers First

Wrong:

```c
#include <stdio.h>
#include <stdlib.h>
#include "uvhttp_server.h"
#include "uvhttp_config.h"
```

Correct:

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#include <stdio.h>
#include <stdlib.h>
```

### Mistake 2: Not Alphabetical

Wrong:

```c
#include "uvhttp_utils.h"
#include "uvhttp_config.h"
#include "uvhttp_server.h"
```

Correct:

```c
#include "uvhttp_config.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"
```

### Mistake 3: Conditional Compilation Includes Mixed In The Middle

Wrong:

```c
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
#include "uvhttp_config.h"
```

Correct:

```c
#include "uvhttp_server.h"
#include "uvhttp_config.h"
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

## Tool Support

### clang-format

The project runs `clang-format` over its code, include order included.

Configuration file: `.clang-format`

### Justfile Commands

```bash
# Format every C source and header
just fmt

# Check style without writing
just check-style
```

## CI/CD Integration

A format check can join the CI/CD workflow:

```yaml
- name: Check code format
  run: |
    clang-format --dry-run --Werror $(find src include test -name '*.c' -o -name '*.h')
```

## Reference Resources

- [Google C++ Style Guide - Names and Order of Includes](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes)
- [clang-format - IncludeCategories](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#includecategories)

---

**Last Updated**: 2026-01-29
**Maintainer**: UVHTTP Development Team
**License**: MIT License
