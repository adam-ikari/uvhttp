/*
 * libFuzzer harness for UVHTTP static-file path resolution.
 *
 * uvhttp_static_resolve_safe_path() is the only public entry point that takes
 * an attacker-controlled URL path and turns it into a filesystem path. The
 * attacker reaches it through the static file route, so every way it maps an
 * input onto the filesystem is worth fuzzing.
 *
 * The harness builds a fixed directory tree once, in LLVMFuzzerInitialize,
 * containing exactly the shapes that make containment checks interesting:
 *
 *   <tmp>/root/pub.txt            a plain file
 *   <tmp>/root/sub/s.txt          a file one level down
 *   <tmp>/root/escape.txt         a SYMLINK pointing OUTSIDE root
 *   <tmp>/root/escapedir          a SYMLINK to a directory OUTSIDE root
 *   <tmp>/outside/secret.txt      the file the symlinks aim at
 *
 * The whole fuzzer input is the candidate path. Consuming bytes to vary the
 * root would cost path-space coverage, and the path is the only part an
 * attacker controls — the root comes from server configuration.
 *
 * What this does NOT assert: that the result is a regular file. It is not
 * required to be one. uvhttp_static_handle_request() feeds the resolved path
 * back through get_file_info(), and when that reports "not a regular file" it
 * checks S_ISDIR to serve a directory listing or an index file. "/" and "/sub"
 * legitimately resolving to their directories is how the static server is
 * meant to work.
 *
 * The single invariant asserted is containment: whenever the function reports
 * success, the path it produced must still be inside the root. That is what a
 * naive prefix-string comparison gets wrong, and what a symlink planted
 * inside the root defeats. Everything else — rejection, directories, nested
 * files — is the function working as designed.
 *
 * Build (clang + libFuzzer + ASan). The library must be built with
 * -DBUILD_WITH_STATIC_FILES=ON, because the option defaults to OFF and the
 * symbol is not compiled otherwise:
 *   cmake -B build_fuzz_static -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON \
 *     -DBUILD_WITH_STATIC_FILES=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
 *   cmake --build build_fuzz_static -j$(nproc) --target uvhttp
 *   clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
 *     -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
 *     -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
 *     test/fuzz/fuzz_static_path.c \
 *     build_fuzz_static/dist/lib/libuvhttp.a \
 *     build_fuzz_static/dist/lib/libminiz.a deps/xxhash/libxxhash.a \
 *     -Wl,--start-group \
 *     deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
 *     deps/mbedtls/build/library/libmbedtls.a \
 *     deps/mbedtls/build/library/libmbedx509.a \
 *     deps/mbedtls/build/library/libmbedcrypto.a \
 *     deps/libuv/build/libuv.a \
 *     -Wl,--end-group -lpthread -lm -ldl -o fuzz_static_path
 *
 * Run:
 *   ./fuzz_static_path -max_total_time=60 -max_len=256
 */

#include "uvhttp_features.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if !UVHTTP_FEATURE_STATIC_FILES
/* Only meaningful when the feature is compiled in. ci-fuzz builds a
 * dedicated libuvhttp.a with -DBUILD_WITH_STATIC_FILES=ON for this target. */
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    (void)data;
    (void)size;
    return 0;
}
#else

#    include "uvhttp_static.h"

static char g_tree[256];
static char g_root[320];

static void WriteFile(const char* rel, const char* contents) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_tree, rel);
    FILE* f = fopen(path, "wb");
    if (f) {
        fputs(contents, f);
        fclose(f);
    }
}

static void MakeLink(const char* link_rel, const char* target) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_tree, link_rel);
    (void)unlink(path);
    (void)symlink(target, path);
}

static void MakeDir(const char* rel) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_tree, rel);
    (void)mkdir(path, 0755);
}

int LLVMFuzzerInitialize(int* argc, char*** argv) {
    (void)argc;
    (void)argv;

    snprintf(g_tree, sizeof(g_tree), "/tmp/uvhttp_fuzz_static_%d",
             (int)getpid());
    (void)mkdir(g_tree, 0755);

    MakeDir("root");
    MakeDir("root/sub");
    MakeDir("outside");

    WriteFile("root/pub.txt", "public\n");
    WriteFile("root/sub/s.txt", "nested\n");
    WriteFile("outside/secret.txt", "SECRET\n");

    /* Symlinks are what a prefix-string containment check gets wrong: the
     * path lexically starts inside root, but resolves outside it. */
    {
        char secret[512];
        snprintf(secret, sizeof(secret), "%s/outside/secret.txt", g_tree);
        MakeLink("root/escape.txt", secret);

        char outdir[512];
        snprintf(outdir, sizeof(outdir), "%s/outside", g_tree);
        MakeLink("root/escapedir", outdir);
    }

    snprintf(g_root, sizeof(g_root), "%s/root", g_tree);
    return 0;
}

/* True when `path` is `root` itself or lies underneath it. Both sides are
 * already canonical, so this is a plain string comparison with an explicit
 * separator check (a bare prefix test would accept "<root>-evil"). */
static int IsInsideRoot(const char* real_root, const char* real_path) {
    size_t root_len = strlen(real_root);
    if (strncmp(real_path, real_root, root_len) != 0) {
        return 0;
    }
    return real_path[root_len] == '/' || real_path[root_len] == '\0';
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0 || size >= UVHTTP_MAX_PATH_SIZE) {
        return 0;
    }
    /* An embedded NUL would silently truncate the path under test, so the
     * run would measure something other than what the fuzzer asked for. */
    if (memchr(data, '\0', size) != NULL) {
        return 0;
    }

    char candidate[UVHTTP_MAX_PATH_SIZE];
    memcpy(candidate, data, size);
    candidate[size] = '\0';

    char resolved[512];
    resolved[0] = '\0';
    int ret = uvhttp_static_resolve_safe_path(g_root, candidate, resolved,
                                              sizeof(resolved));

    if (ret != 1) {
        return 0; /* rejected — always safe */
    }

    /* Accepted. Canonicalize both sides: the returned string may still
     * contain symlinks or "..", and containment has to hold for the file
     * that is actually opened. */
    char real_root[512];
    char real_resolved[512];
    if (!realpath(g_root, real_root) || !realpath(resolved, real_resolved)) {
        fprintf(stderr,
                "accepted but does not exist: candidate=%s resolved=%s\n",
                candidate, resolved);
        abort();
    }

    if (!IsInsideRoot(real_root, real_resolved)) {
        fprintf(stderr,
                "ESCAPED root: candidate=%s resolved=%s (real=%s root=%s)\n",
                candidate, resolved, real_resolved, real_root);
        abort();
    }

    return 0;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
