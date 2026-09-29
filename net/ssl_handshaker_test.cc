#include <openssl/ssl.h>

#include "net/ssl_handshaker.h"
#include "testing/testing.h"

namespace {

class SslHandshakerTest : public ::testing::Test {};

TEST_F(SslHandshakerTest, HandshakeCompleteOnSuccess) {
  net::SslHandshaker handshaker([](void*) { return 0; });

  EXPECT_TRUE(handshaker.in_progress());

  auto vibe_or = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(vibe_or.IsOk());
  EXPECT_TRUE(vibe_or.ValueOrDie() == net::SslHandshaker::Vibe::kComplete);
  EXPECT_FALSE(handshaker.in_progress());
}

TEST_F(SslHandshakerTest, HandshakeWantsRead) {
  net::SslHandshaker handshaker([](void*) { return SSL_ERROR_WANT_READ; });

  EXPECT_TRUE(handshaker.in_progress());

  auto vibe_or = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(vibe_or.IsOk());
  EXPECT_TRUE(vibe_or.ValueOrDie() == net::SslHandshaker::Vibe::kWantedRead);
  EXPECT_TRUE(handshaker.wants_read());
  EXPECT_FALSE(handshaker.wants_write());
  EXPECT_TRUE(handshaker.in_progress());
}

TEST_F(SslHandshakerTest, HandshakeWantsWrite) {
  net::SslHandshaker handshaker([](void*) { return SSL_ERROR_WANT_WRITE; });

  EXPECT_TRUE(handshaker.in_progress());
  EXPECT_FALSE(handshaker.wants_write());

  auto vibe_or = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(vibe_or.IsOk());
  EXPECT_TRUE(vibe_or.ValueOrDie() == net::SslHandshaker::Vibe::kWantedWrite);
  EXPECT_FALSE(handshaker.wants_read());
  EXPECT_TRUE(handshaker.wants_write());
  EXPECT_TRUE(handshaker.in_progress());
}

TEST_F(SslHandshakerTest, MultiStepHandshakeProgression) {
  int call_count = 0;
  net::SslHandshaker handshaker([&call_count](void*) {
    if (call_count++ == 0) return SSL_ERROR_WANT_READ;
    if (call_count == 2) return SSL_ERROR_WANT_WRITE;
    return 0;
  });

  auto v1 = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(v1.IsOk());
  EXPECT_TRUE(v1.ValueOrDie() == net::SslHandshaker::Vibe::kWantedRead);
  EXPECT_TRUE(handshaker.wants_read());

  auto v2 = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(v2.IsOk());
  EXPECT_TRUE(v2.ValueOrDie() == net::SslHandshaker::Vibe::kWantedWrite);
  EXPECT_TRUE(handshaker.wants_write());

  auto v3 = handshaker.RunAccept(nullptr);
  ASSERT_TRUE(v3.IsOk());
  EXPECT_TRUE(v3.ValueOrDie() == net::SslHandshaker::Vibe::kComplete);
  EXPECT_FALSE(handshaker.in_progress());
}

}  // namespace
