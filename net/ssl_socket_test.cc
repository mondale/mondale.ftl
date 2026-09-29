#include <openssl/ssl.h>

#include "core/syscalls.h"
#include "io/io.h"
#include "net/socket_address.h"
#include "net/ssl_context.h"
#include "net/ssl_socket.h"
#include "testing/testing.h"

namespace {

class SslSocketTest : public ::testing::Test {
 protected:
  std::unique_ptr<io::Epoller> e_ = io::Epoller::Build(1).ValueOrDie();
  std::unique_ptr<net::SslContext> sslc_ =
      net::SslContext::BuildServer().ValueOrDie();
  net::SocketAddress sa_ =
      net::SocketAddress::FromString("1.2.3.4:5678").ValueOrDie();
};

TEST_F(SslSocketTest, HelloWorldWithSsl) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto ssl_socket = net::SslSocket::New(e_.get(), sslc_.get(), std::move(s1),
                                        sa_, net::SslHandshaker{})
                        .ValueOrDie();

  // Post the response message.
  constexpr char kMessage[] = "You are thusly greeted.";
  Notification sent;  // Note that on a failure this test case can segfault.
  ssl_socket->Post(
      io::SourceBuffer(kMessage, sizeof(kMessage), [&]() { sent.Notify(); }));

  // Post the buffering for the inbound message;
  char inbound[64];
  std::atomic<size_t> bytes_received{0};
  Notification received;  // Note that on a failure this test case can segfault.
  ssl_socket->Post(io::SinkBuffer(inbound, 64, [&](size_t bytes) {
    bytes_received.store(bytes, std::memory_order_release);
    received.Notify();
  }));

  // This test case uses synchronous syscalls/SSL calls to send "Hello, world!"
  // and then receive "You are thusly greeted." from the server.

  // Set up a synchronous client-side SSL connection over the other socket pair
  // end (s0)
  auto* client_ctx = SSL_CTX_new(TLS_client_method());
  ASSERT_TRUE(client_ctx != nullptr);
  auto* client_ssl = SSL_new(client_ctx);
  ASSERT_TRUE(client_ssl != nullptr);

  ASSERT_EQ(SSL_set_fd(client_ssl, s0.fd()), 1);

  // Perform the synchronous TLS handshake
  ASSERT_EQ(SSL_connect(client_ssl), 1);

  // Client sends "Hello, world!" to the server
  constexpr char kClientHello[] = "Hello, world!";
  const int written = SSL_write(client_ssl, kClientHello, sizeof(kClientHello));
  ASSERT_GT(written, 0);

  // Wait for the server to receive the inbound message from the client
  received.WaitForNotification();
  EXPECT_EQ(bytes_received.load(std::memory_order_acquire),
            sizeof(kClientHello));
  EXPECT_THAT(inbound, testing::HasSubstr("Hello, world!"));

  // Client reads the response sent by the server
  char client_response[64];
  const int read_bytes =
      SSL_read(client_ssl, client_response, sizeof(client_response));
  ASSERT_GT(read_bytes, 0);

  // Wait for the server's send operation completion notification
  sent.WaitForNotification();

  // Verify the response matches what the server posted
  std::string_view response_view(client_response, read_bytes);
  EXPECT_THAT(response_view, testing::HasSubstr("You are thusly greeted."));

  // Cleanup client-side OpenSSL resources
  SSL_free(client_ssl);
  SSL_CTX_free(client_ctx);
}

}  // namespace
