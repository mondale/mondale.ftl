#include <openssl/ssl.h>

#include "net/ssl_context.h"
#include "testing/testing.h"

namespace net {

class SslContextTest : public ::testing::Test {};

TEST_F(SslContextTest, BuildServerSuccess) {
  auto server_ctx_or = net::SslContext::BuildServer();
  ASSERT_TRUE(server_ctx_or.IsOk());

  auto server_ctx = std::move(server_ctx_or).ValueOrDie();

  // Verify we can allocate SSL handles from the context
  auto ssl_or = server_ctx->AllocateSsl();
  EXPECT_TRUE(ssl_or.IsOk());

  // Clean up the allocated ssl handle
  if (ssl_or.IsOk()) {
    SSL_free(static_cast<SSL*>(ssl_or.ValueOrDie()));
  }
}

TEST_F(SslContextTest, BuildClientSuccess) {
  auto client_ctx_or = net::SslContext::BuildClient();
  ASSERT_TRUE(client_ctx_or.IsOk());

  auto client_ctx = std::move(client_ctx_or).ValueOrDie();
  auto ssl_or = client_ctx->AllocateSsl();
  EXPECT_TRUE(ssl_or.IsOk());

  if (ssl_or.IsOk()) {
    SSL_free(static_cast<SSL*>(ssl_or.ValueOrDie()));
  }
}

}  // namespace net
