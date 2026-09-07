# Embedding Guide

This guide walks a **new embedder** — someone adding uvhttp to their own CMake
project for the first time — through the full integration: prerequisites,
each integration method, a minimal working server, feature trimming, and
troubleshooting.

> A runnable, self-contained example project lives in
> [`examples/embedding/`](../../examples/embedding/). Everything in this guide
> is reflected there — use it as a template.

---

## 1. Prerequisites

- **Linux** (the currently supported platform).
- **CMake ≥ 3.10** and a **C11** compiler (GCC or Clang).
- uvhttp's dependencies live under `deps/` as **git submodules**. You must
  clone recursively:

  ```bash
  git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
  ```

  If you already cloned without `--recurse-submodules`:

  ```bash
  git submodule update --init --recursive
  ```

- uvhttp is a **static library**. It bundles everything it needs: libuv,
  llhttp, xxhash, mbedtls, and uthash are compiled as part of the build, so you
  do **not** need to install them separately.

---

## 2. Integration methods

There are three ways to bring uvhttp into your project. All produce a `uvhttp`
CMake target (static library) that you link against.

| Method | Best for | Setup cost |
|--------|----------|-----------|
| `add_subdirectory` | vendoring uvhttp inside your tree (submodule or copy) | Low — recommended |
| `FetchContent` | fetching uvhttp automatically at configure time | Low |
| system install + `find_package` | sharing one uvhttp build across projects | Medium (install once) |

### Method A — `add_subdirectory` (recommended)

Put uvhttp in your tree (e.g. `third_party/uvhttp`), then:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

set(CMAKE_C_STANDARD 11)

# Point at your uvhttp checkout
add_subdirectory(third_party/uvhttp)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp)
```

### Method B — `FetchContent`

```cmake
cmake_minimum_required(VERSION 3.14)
project(myapp C)

include(FetchContent)
FetchContent_Declare(uvhttp
  GIT_REPOSITORY https://github.com/adam-ikari/uvhttp.git
  GIT_TAG        v2.7.1
)
FetchContent_MakeAvailable(uvhttp)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp)
```

> FetchContent clones the repo **without submodules by default**. uvhttp's
> `deps/` submodules are populated by its own CMakeLists when present, so pin
> to a tagged release and, if a submodule issue arises, add
> `FetchContent_Populate` + `git submodule update --init --recursive` steps in
> the checkout directory. `add_subdirectory` (Method A) is the more robust
> choice when you control the checkout.

### Method C — system install + `find_package`

Install once, then use the exported CMake package:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
cmake --install . --prefix /usr/local
```

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(uvhttp REQUIRED)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp::uvhttp)
```

---

## 3. Minimal server

Create `main.c` (this exact file is in `examples/embedding/main.c`):

```c
#include "uvhttp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 应用上下文：通过 loop 关联，避免全局变量（uvhttp 设计原则） */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

/* 信号句柄，用于优雅退出 */
static uv_signal_t g_sigint;
static uv_signal_t g_sigterm;
static app_context_t g_app; /* 单实例示例用静态上下文 */

static void on_signal(uv_signal_t* handle, int signum) {
    (void)signum;
    /* 停止事件循环；uv_signal 会唤醒正在阻塞的 loop */
    uv_stop(handle->loop);
    printf("\nReceived signal, shutting down...\n");
    fflush(stdout);
}

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    const char* body = "Hello from embedded uvhttp!\n";
    uvhttp_response_set_body(res, body, strlen(body));
    return (int)uvhttp_response_send(res);
}

int main(int argc, char** argv) {
    /* 端口可通过命令行参数指定，默认 8080 */
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    uv_loop_t* loop = uv_default_loop();
    if (loop == NULL) {
        fprintf(stderr, "error: cannot create libuv event loop\n");
        return 1;
    }

    if (uvhttp_server_new(loop, &g_app.server) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create uvhttp server\n");
        return 1;
    }
    if (uvhttp_router_new(&g_app.router) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create router\n");
        uvhttp_server_free(g_app.server);
        return 1;
    }
    if (uvhttp_router_add_route(g_app.router, "/", hello_handler) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot add route /\n");
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }
    uvhttp_server_set_router(g_app.server, g_app.router);

    uv_signal_init(loop, &g_sigint);
    uv_signal_start(&g_sigint, on_signal, SIGINT);
    uv_signal_init(loop, &g_sigterm);
    uv_signal_start(&g_sigterm, on_signal, SIGTERM);

    if (uvhttp_server_listen(g_app.server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot listen on 0.0.0.0:%d\n", port);
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }

    printf("Embedded uvhttp listening on http://0.0.0.0:%d\n", port);
    printf("Test: curl http://localhost:%d/\n", port);

    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理：
     * 1. 先关闭自建的 uv_signal 句柄——uvhttp_server_free 内部会跑
     *    UV_RUN_ONCE 清理循环，loop 中残留活跃句柄会使其阻塞。
     * 2. uvhttp_server_free 会一并释放其持有的 router 与 tcp_handle，
     *    因此不要重复调用 uvhttp_router_free。
     */
    uv_close((uv_handle_t*)&g_sigint, NULL);
    uv_close((uv_handle_t*)&g_sigterm, NULL);
    uvhttp_server_free(g_app.server);
    printf("Shutdown complete.\n");
    fflush(stdout);
    return 0;
}
```

Build and run:

```bash
cmake -S . -B build
cmake --build build -j
./build/embedding_server   # or ./build/myapp
```

Verify from another terminal:

```bash
curl http://localhost:8080/
# Hello from embedded uvhttp!
```

---

## 4. Feature trimming

All features are compile-time options. Pass them at configure time:

```bash
cmake -DBUILD_WITH_WEBSOCKET=OFF -DBUILD_WITH_HTTPS=OFF ..
```

| Option | Default | Effect |
|--------|---------|--------|
| `BUILD_WITH_WEBSOCKET` | ON | WebSocket support (RFC 6455) |
| `BUILD_WITH_HTTPS` | ON | HTTPS/TLS via mbedtls |
| `BUILD_WITH_STATIC_FILES` | OFF | Static file serving (sendfile) |
| `BUILD_WITH_LRU_CACHE` | ON | Static file / compression LRU cache |
| `BUILD_WITH_ROUTER_CACHE` | OFF | Router lookup cache |
| `BUILD_WITH_COMPRESSION` | ON | gzip response compression |
| `BUILD_WITH_MIMALLOC` | OFF* | mimalloc allocator (`UVHTTP_ALLOCATOR_TYPE=1`) |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | Allocator: `0` system, `1` mimalloc, `2` custom |
| `BUILD_EXAMPLES` | OFF | uvhttp's own examples |
| `BUILD_BENCHMARKS` | ON | uvhttp's benchmarks (disable when embedding) |
| `BUILD_TESTS` | ON | uvhttp's unit/integration tests (disable when embedding) |

\* `BUILD_WITH_MIMALLOC` defaults depend on `UVHTTP_ALLOCATOR_TYPE`; see
`CMakeLists.txt` for the exact rule.

When embedding, also pass:

```bash
cmake -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF ..
```

See [Build Configuration Matrix](./BUILD_CONFIGURATION_MATRIX.md) and
[Advanced Build Options](./ADVANCED_BUILD_OPTIONS.md) for the full option set.

---

## 5. Verifying your integration

1. **Compile** — your project builds with uvhttp linked statically.
2. **Smoke test** — start the server, `curl` a route, confirm the response.
3. **Clean shutdown** — send SIGINT/SIGTERM; the process exits without leaks.
4. **Memory safety** — rebuild with sanitizers and rerun:

   ```bash
   cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
   ```

5. Run uvhttp's own suite once to confirm the library is healthy:

   ```bash
   cd <uvhttp>/build && ctest --output-on-failure
   ```

For the full per-release checklist, see
[Embedding Checklist](../embedding-checklist.md) (section 9 is written
specifically for first-time embedders).

---

## 6. Troubleshooting

| Symptom | Cause / Fix |
|---------|-------------|
| `fatal error: llhttp.h: No such file or directory` (or `uv.h`, `uthash.h`, mbedtls headers) | You are on a version before **v2.7.1**. Public header dependencies were made `PUBLIC` in v2.7.1 so they propagate to embedders. Upgrade to v2.7.1+. |
| `undefined reference to <symbol>` at link | The `uvhttp` target already links libuv/llhttp/xxhash/mbedtls transitively — do **not** also add them manually. Missing `-lpthread -lm -ldl` on old CMake/older toolchains: `target_link_libraries(myapp PRIVATE uvhttp pthread m dl)`. |
| Empty `deps/` directory | Submodules not initialized. Run `git submodule update --init --recursive` inside the uvhttp checkout. |
| Build is slow / many uvhttp test targets appear | uvhttp's test targets build unconditionally. Disable what you can with `-DBUILD_BENCHMARKS=OFF`; the tests do not affect your binary. |
| `mbedtls` or mimalloc enabled when unwanted | Trim with `-DBUILD_WITH_HTTPS=OFF` / `-DBUILD_WITH_WEBSOCKET=OFF` / `-DBUILD_WITH_MIMALLOC=OFF`. |

---

## 7. Next steps

- Run the live example: [`examples/embedding/`](../../examples/embedding/)
- Embedded profiles & configuration: [Embedded Profile](../embedded-profile.md)
- CMake specifics: [CMake Integration](./INSTALL_CMAKE.md),
  [CMake Configuration](./CMAKE_CONFIGURATION.md)
- Per-release verification: [Embedding Checklist](../embedding-checklist.md)
