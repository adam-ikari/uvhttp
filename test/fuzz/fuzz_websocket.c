/*
 * libFuzzer harness for UVHTTP WebSocket frame parsing.
 *
 * Feeds arbitrary byte sequences through the two RFC 6455 frame codecs that
 * consume raw wire bytes:
 *
 *   - uvhttp_ws_parse_frame_header(): decodes the 2/4/10-byte frame header,
 *     including the 64-bit extended length path and the RFC 6455 §5.2 rule
 *     that the top bit of a 64-bit length MUST be 0 (rejected as a protocol
 *     error). The header structure uses bitfields, so this also exercises the
 *     packing/truncation edge cases of the 7-bit payload_len vs the full
 *     payload_length.
 *
 *   - uvhttp_ws_apply_mask(): XORs the payload in place against a 4-byte
 *     masking key (RFC 6455 §5.3). The loop index-wraps on key[i % 4].
 *
 * Both are pure buffer → structure transforms with no socket or connection
 * state, so each fuzzer iteration is two small calls with no allocation.
 *
 * Build (clang + libFuzzer + ASan):
 *   clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
 *     -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
 *     -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
 *     test/fuzz/fuzz_websocket.c \
 *     build_fuzz/dist/lib/libuvhttp.a build_fuzz/dist/lib/libminiz.a \
 *     deps/xxhash/libxxhash.a \
 *     -Wl,--start-group \
 *     deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
 *     deps/mbedtls/build/library/libmbedtls.a \
 *     deps/mbedtls/build/library/libmbedx509.a \
 *     deps/mbedtls/build/library/libmbedcrypto.a \
 *     deps/libuv/build/libuv.a \
 *     -Wl,--end-group -lpthread -lm -ldl -o fuzz_websocket
 *
 * Run:
 *   ./fuzz_websocket -max_total_time=60 -max_len=256
 */
#include "uvhttp_features.h"
#include "uvhttp_websocket.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    /* Parse as a frame header. len < 2 / < 4 / < 10 are rejected inside, so
     * small inputs exercise the early-return paths and larger ones the
     * extended-length decoding. */
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    (void)uvhttp_ws_parse_frame_header(data, size, &header, &header_size);

    /* Decode the mask on a copy of the input (apply_mask is in-place), using
     * the header's own 4 reserved bytes as the masking key when available. */
    uint8_t key[4] = {0x11, 0x22, 0x33, 0x44};
    if (header_size >= 6 && data) {
        if (size >= header_size + 4) {
            memcpy(key, data + 2, 4);
        }
    }

    /* Apply the mask to a payload region derived from the input: anything
     * after the header, bounded by size. */
    if (size > header_size) {
        uint8_t buf[64];
        size_t payload_len = size - header_size;
        if (payload_len > sizeof(buf)) {
            payload_len = sizeof(buf);
        }
        memcpy(buf, data + header_size, payload_len);
        uvhttp_ws_apply_mask(buf, payload_len, key);
    }

    return 0;
}