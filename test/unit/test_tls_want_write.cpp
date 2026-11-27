/*
 * A TLS handshake write that hits EAGAIN used to be folded into WANT_READ:
 * the code then waited only for socket readability. The peer is itself
 * waiting for the server's handshake record, so the connection deadlocked
 * until the timeout. The fix keeps WANT_WRITE distinct and arms a writable
 * poll. The mapping is asserted directly here because driving a real
 * handshake into EAGAIN requires a full socket send buffer, which a mock
 * cannot reach (mbedtls reads before writing on a fresh connection).
 */

#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"

#include <gtest/gtest.h>

#if UVHTTP_FEATURE_TLS
extern "C" {
#include <mbedtls/ssl.h>
}
#endif

namespace {

TEST(TlsWantWriteTest, WantWriteIsNotFoldedIntoWantRead) {
#if UVHTTP_FEATURE_TLS
    EXPECT_EQ(uvhttp_tls_map_handshake_result(0), UVHTTP_OK);
    EXPECT_EQ(uvhttp_tls_map_handshake_result(MBEDTLS_ERR_SSL_WANT_READ),
              UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_EQ(uvhttp_tls_map_handshake_result(MBEDTLS_ERR_SSL_WANT_WRITE),
              UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_EQ(uvhttp_tls_map_handshake_result(MBEDTLS_ERR_SSL_INTERNAL_ERROR),
              UVHTTP_ERROR_TLS_HANDSHAKE);
#else
    GTEST_SKIP() << "TLS not compiled in";
#endif
}

}  // namespace
