# Forward Declaration and Circular Dependency Guidelines

This document defines the forward declaration and circular dependency guidelines for the UVHTTP project, ensuring code maintainability and compilation efficiency.

## Overview

Forward declarations and circular dependencies are common design issues in C/C++ projects. Using forward declarations correctly avoids unnecessary includes, reduces compilation time, and improves code maintainability.

## Forward Declaration Guidelines

### When to Use Forward Declarations

**Cases where forward declarations should be used**:

1. **Only a pointer or reference is needed in the header file**
   ```c
   // Correct
   typedef struct uvhttp_server uvhttp_server_t;
   
   struct uvhttp_connection {
       struct uvhttp_server* server;  // Only a pointer is needed
   };
   ```

2. **To avoid circular dependencies**
   ```c
   // uvhttp_server.h
   typedef struct uvhttp_connection uvhttp_connection_t;
   
   // uvhttp_connection.h
   typedef struct uvhttp_server uvhttp_server_t;
   ```

3. **Using pointer parameters in function declarations**
   ```c
   // Correct
   uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                        uvhttp_connection_t** conn);
   ```

**Cases where forward declarations should not be used**:

1. **When members need to be accessed**
   ```c
   // Incorrect
   typedef struct uvhttp_request uvhttp_request_t;
   
   void func(uvhttp_request_t* req) {
       req->method = UVHTTP_GET;  // Members need to be accessed
   }
   
   // Correct
   #include "uvhttp_request.h"
   ```

2. **When struct methods need to be called**
   ```c
   // Incorrect
   typedef struct uvhttp_response uvhttp_response_t;
   
   void func(uvhttp_response_t* resp) {
       uvhttp_response_set_status(resp, 200);  // Methods need to be called
   }
   
   // Correct
   #include "uvhttp_response.h"
   ```

3. **When a complete type definition is needed**
   ```c
   // Incorrect
   typedef struct uvhttp_request uvhttp_request_t;
   
   void func() {
       uvhttp_request_t req;  // A complete type is needed
   }
   
   // Correct
   #include "uvhttp_request.h"
   ```

### Forward Declaration Syntax

```c
// Basic type
typedef struct uvhttp_server uvhttp_server_t;

// Pointer type
typedef struct uvhttp_server* uvhttp_server_ptr_t;

// Function pointer type
typedef int (*uvhttp_handler_t)(void* context);

// Within a struct
struct uvhttp_connection {
    struct uvhttp_server* server;
    uvhttp_request_t* request;
    uvhttp_response_t* response;
};
```

## Circular Dependency Guidelines

### Types of Circular Dependencies

#### Type 1: A includes B, and B includes A

```c
// uvhttp_server.h
#include "uvhttp_connection.h"  // Circular reference

// uvhttp_connection.h
#include "uvhttp_server.h"  // Circular reference
```

#### Type 2: A includes B, B includes C, and C includes A

```c
// uvhttp_server.h
#include "uvhttp_connection.h"

// uvhttp_connection.h
#include "uvhttp_request.h"

// uvhttp_request.h
#include "uvhttp_server.h"  // Circular reference
```

### Methods for Resolving Circular Dependencies

#### Method 1: Use Forward Declarations (Recommended)

```c
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;  // Forward declaration

struct uvhttp_server {
    uvhttp_connection_t* connections;
};

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t;  // Forward declaration

struct uvhttp_connection {
    struct uvhttp_server* server;
};
```

#### Method 2: Extract a Common Interface into a Separate Header File

```c
// uvhttp_common.h (common interface)
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;

// uvhttp_server.h
#include "uvhttp_common.h"  // Uses the common interface

// uvhttp_connection.h
#include "uvhttp_common.h"  // Uses the common interface
```

#### Method 3: Use void* Pointers (Not Recommended)

```c
// Not recommended: loses type safety
struct uvhttp_connection {
    void* server;  // Loses type information
};
```

## Practical Application in the UVHTTP Project

### Resolved Circular Dependencies

#### 1. uvhttp_server.h and uvhttp_connection.h

**Before the fix**:
```c
// uvhttp_server.h
#include "uvhttp_connection.h"  // Circular reference

// uvhttp_connection.h
#include "uvhttp_server.h"  // Circular reference
```

**After the fix**:
```c
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;  // Forward declaration

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t;  // Forward declaration
```

#### 2. uvhttp_response.h

**Before the fix**:
```c
#include "uvhttp_connection.h"  // Could cause a circular reference
```

**After the fix**:
```c
typedef struct uvhttp_connection uvhttp_connection_t;  // Forward declaration
```

### Current Forward Declaration Usage

| Header File | Forward-Declared Types | Purpose |
|--------|-------------|------|
| uvhttp_common.h | uvhttp_request_t, uvhttp_response_t | Defines request handler types |
| uvhttp_response.h | uvhttp_connection_t, uvhttp_response_t | Avoids circular references |
| uvhttp_utils.h | uvhttp_response_t | Function parameters |
| uvhttp_router.h | uvhttp_response_t | Function parameters |
| uvhttp_server.h | uvhttp_request_t, uvhttp_response_t, uvhttp_connection_t, uvhttp_router_t, uvhttp_connection_t | Avoids circular references |
| uvhttp_connection.h | uvhttp_connection_t, uvhttp_server_t | Avoids circular references |

## Best Practices

### 1. Header File Inclusion Order

```c
// 1. The corresponding header file (if any)
#include "uvhttp_server.h"

// 2. UVHTTP project header files (alphabetical order)
#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"

// 3. Other project-internal header files (alphabetical order)
#include "uthash.h"

// 4. Standard library header files (alphabetical order)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 5. Third-party library header files (alphabetical order)
#include <uv.h>

// 6. Forward declarations (alphabetical order)
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 7. Conditional compilation includes
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket.h"
#endif
```

### 2. Include Complete Definitions in .c Files

```c
// Correct: include complete definitions in the .c file
#include "uvhttp_server.h"
#include "uvhttp_connection.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

// All members can be accessed in the .c file
void some_function() {
    uvhttp_server_t* server = ...;
    uvhttp_connection_t* conn = ...;
    conn->server = server;  // Members can be accessed
}
```

### 3. Use Forward Declarations in .h Files

```c
// Correct: use forward declarations in the .h file
typedef struct uvhttp_server uvhttp_server_t;
typedef struct uvhttp_connection uvhttp_connection_t;

struct uvhttp_connection {
    struct uvhttp_server* server;  // Only a pointer is needed
    uvhttp_request_t* request;
    uvhttp_response_t* response;
};
```

### 4. Use a Common Interface Header File

```c
// uvhttp_common.h (common interface)
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef int (*uvhttp_request_handler_t)(uvhttp_request_t*, uvhttp_response_t*);

// Other header files
#include "uvhttp_common.h"  // Uses the common interface
```

## Detection Tools

### 1. Compiler Warnings

```bash
# GCC
gcc -Wmissing-include-dirs -Werror

# Clang
clang -Wmissing-include-dirs -Werror
```

### 2. Static Analysis Tools

```bash
# cppcheck
cppcheck --enable=missingInclude src/ include/

# clang-tidy
clang-tidy src/*.c include/*.h
```

### 3. Dependency Analysis Tools

```bash
# include-what-you-use
include-what-you-use src/*.c include/*.h
```

## Common Mistakes

### Mistake 1: Accessing Members in a Header File

```c
// Incorrect
typedef struct uvhttp_request uvhttp_request_t;

void func(uvhttp_request_t* req) {
    req->method = UVHTTP_GET;  // A complete definition is required
}

// Correct
#include "uvhttp_request.h"

void func(uvhttp_request_t* req) {
    req->method = UVHTTP_GET;  // Members can be accessed
}
```

### Mistake 2: Circular Dependencies Causing Compile Errors

```c
// Incorrect: circular reference
// uvhttp_server.h
#include "uvhttp_connection.h"

// uvhttp_connection.h
#include "uvhttp_server.h"

// Correct: use forward declarations
// uvhttp_server.h
typedef struct uvhttp_connection uvhttp_connection_t;

// uvhttp_connection.h
typedef struct uvhttp_server uvhttp_server_t
```

### Mistake 3: Overusing Forward Declarations

```c
// Incorrect: overusing forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;
typedef struct uvhttp_router uvhttp_router_t;

void func() {
    uvhttp_request_t req;  // A complete definition is required
    uvhttp_response_t resp;
    uvhttp_connection_t conn;
    // ...
}

// Correct: include complete definitions in the .c file
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_connection.h"

void func() {
    uvhttp_request_t req;  // The complete type can be used
    uvhttp_response_t resp;
    uvhttp_connection_t conn;
    // ...
}
```

## Performance Impact

### Compilation Time

- **Using forward declarations**: reduces unnecessary includes and speeds up compilation
- **Circular references**: increase compilation time and may cause infinite recursion

### Binary Size

- **Using forward declarations**: no impact on binary size
- **Circular references**: no impact on binary size (only affects compilation)

### Runtime Performance

- **Using forward declarations**: no impact
- **Circular references**: no impact

## Reference Resources

- [Google C++ Style Guide - Forward Declarations](https://google.github.io/styleguide/cppguide.html#Forward_Declarations)
- [C++ FAQ - What are forward declarations?](https://isocpp.org/wiki/faq/forward-declarations)
- [Include Guards and Forward Declarations](https://en.cppreference.com/w/cpp/header/include)

---

**Last updated**: 2026-01-29
**Maintainer**: UVHTTP development team
**License**: MIT License
