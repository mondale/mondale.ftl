#include <openssl/ssl.h>

#include "core/idioms.h"
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
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&]() {
    ssl_socket->Post(
        io::SourceBuffer(kMessage, sizeof(kMessage), [&]() { sent.Notify(); }));
  }));

  // Post the buffering for the inbound message;
  char inbound[64];
  std::atomic<size_t> bytes_received{0};
  Notification received;  // Note that on a failure this test case can segfault.
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&]() {
    ssl_socket->Post(io::SinkBuffer(inbound, 64, [&](size_t bytes) {
      bytes_received.store(bytes, std::memory_order_release);
      received.Notify();
    }));
  }));

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

  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&]() { ssl_socket->PostClose(); }));

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

TEST_F(SslSocketTest, AbruptClientDisconnect) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto ssl_socket = net::SslSocket::New(e_.get(), sslc_.get(), std::move(s1),
                                        sa_, net::SslHandshaker{})
                        .ValueOrDie();

  char inbound[64];
  Notification received;  // Note that on a failure this test case can segfault.
  std::atomic<size_t> bytes_received{1};
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&, s = ssl_socket]() {
    s->Post(io::SinkBuffer(inbound, 64, [&](size_t bytes) {
      bytes_received.store(bytes, std::memory_order_release);
      received.Notify();
    }));
  }));

  // Drop our ref on the socket.
  {
    auto c = std::move(ssl_socket);
  }

  // Abruptly close the client-side socket immediately without handshaking
  {
    auto c = std::move(s0);
  }

  received.WaitForNotification();
  ASSERT_EQ(0, bytes_received.load(std::memory_order_acquire));
}

TEST_F(SslSocketTest, HandshakeFailureProtocolMismatch) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto ssl_socket = net::SslSocket::New(e_.get(), sslc_.get(), std::move(s1),
                                        sa_, net::SslHandshaker{})
                        .ValueOrDie();

  // Post a sink buffer to catch the failure/closure outcome
  char inbound[64];
  Notification received;
  std::atomic<size_t> bytes_received{1};
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&, s = ssl_socket]() {
    s->Post(io::SinkBuffer(inbound, 64, [&](size_t bytes) {
      bytes_received.store(bytes, std::memory_order_release);
      received.Notify();
    }));
  }));

  // Send raw garbage bytes instead of a valid TLS ClientHello
  constexpr char kJunkData[] = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  CHECK_OK(core::idioms::WriteExactly(s0, {kJunkData, sizeof(kJunkData)}));

  // Drop our ref on the socket to let the event loop process the handshake
  // failure
  {
    auto c = std::move(ssl_socket);
  }

  // Close client side
  {
    auto c = std::move(s0);
  }

  received.WaitForNotification();
  ASSERT_EQ(0, bytes_received.load(std::memory_order_acquire));
}

TEST_F(SslSocketTest, LargeMultiPacketPayload) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto ssl_socket = net::SslSocket::New(e_.get(), sslc_.get(), std::move(s1),
                                        sa_, net::SslHandshaker{})
                        .ValueOrDie();

  std::string large_message(64 * 1024, 'X');
  Notification sent;
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&]() {
    ssl_socket->Post(io::SourceBuffer(
        large_message.data(), large_message.size(), [&]() { sent.Notify(); }));
  }));

  // Establish synchronous client connection
  auto* client_ctx = SSL_CTX_new(TLS_client_method());
  auto* client_ssl = SSL_new(client_ctx);
  SSL_set_fd(client_ssl, s0.fd());
  ASSERT_EQ(SSL_connect(client_ssl), 1);

  // Read the full 64 KB stream on the client side
  std::string received_data;
  received_data.resize(large_message.size());
  size_t total_read = 0;
  while (total_read < large_message.size()) {
    int n = SSL_read(client_ssl, received_data.data() + total_read,
                     large_message.size() - total_read);
    if (n > 0) {
      total_read += n;
    } else {
      int err = SSL_get_error(client_ssl, n);
      if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
        break;
      }
    }
  }

  EXPECT_EQ(total_read, large_message.size());
  EXPECT_EQ(received_data, large_message);

  sent.WaitForNotification();

  // Drop ref and cleanup
  {
    auto c = std::move(ssl_socket);
  }
  SSL_free(client_ssl);
  SSL_CTX_free(client_ctx);
}

TEST_F(SslSocketTest, ChainedQueuedBuffers) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto ssl_socket = net::SslSocket::New(e_.get(), sslc_.get(), std::move(s1),
                                        sa_, net::SslHandshaker{})
                        .ValueOrDie();

  constexpr char kMsg1[] = "First chunk. ";
  constexpr char kMsg2[] = "Second chunk. ";
  constexpr char kMsg3[] = "Final chunk.";

  Notification n1;
  Notification n2;
  Notification n3;
  CHECK_OK(e_->RunWithAffinity(ssl_socket, [&]() {
    ssl_socket->Post(
        io::SourceBuffer(kMsg1, sizeof(kMsg1) - 1, [&]() { n1.Notify(); }));
    ssl_socket->Post(
        io::SourceBuffer(kMsg2, sizeof(kMsg2) - 1, [&]() { n2.Notify(); }));
    ssl_socket->Post(
        io::SourceBuffer(kMsg3, sizeof(kMsg3) - 1, [&]() { n3.Notify(); }));
  }));

  auto* client_ctx = SSL_CTX_new(TLS_client_method());
  auto* client_ssl = SSL_new(client_ctx);
  SSL_set_fd(client_ssl, s0.fd());
  ASSERT_EQ(SSL_connect(client_ssl), 1);

  char buffer[128];
  size_t total_read = 0;
  const size_t expected_size =
      (sizeof(kMsg1) - 1) + (sizeof(kMsg2) - 1) + (sizeof(kMsg3) - 1);

  while (total_read < expected_size) {
    int n =
        SSL_read(client_ssl, buffer + total_read, sizeof(buffer) - total_read);
    if (n > 0) {
      total_read += n;
    } else {
      int err = SSL_get_error(client_ssl, n);
      if (err != SSL_ERROR_WANT_READ) break;
    }
  }

  n1.WaitForNotification();
  n2.WaitForNotification();
  n3.WaitForNotification();

  std::string_view full_received(buffer, total_read);
  EXPECT_THAT(full_received,
              testing::HasSubstr("First chunk. Second chunk. Final chunk."));

  // Drop ref and cleanup
  {
    auto c = std::move(ssl_socket);
  }
  SSL_free(client_ssl);
  SSL_CTX_free(client_ctx);
}

}  // namespace
