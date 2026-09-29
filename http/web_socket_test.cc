#include "core/idioms.h"
#include "core/syscalls.h"
#include "http/web_socket.h"
#include "io/io.h"
#include "net/socket_address.h"
#include "testing/testing.h"

namespace {

class WebSocketTest : public ::testing::Test {
 protected:
  std::unique_ptr<io::Epoller> e_ = io::Epoller::Build(1).ValueOrDie();
  net::SocketAddress sa_ =
      net::SocketAddress::FromString("1.2.3.4:5678").ValueOrDie();
};

TEST_F(WebSocketTest, HelloWorld) {
  auto [s0, s1] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  auto web_socket =
      http::WebSocket::New(e_.get(), std::move(s1), sa_).ValueOrDie();

  // Post the response message.
  constexpr char kMessage[] = "You are thusly greeted.";
  Notification sent;  // Note that on a failure this test case can segfault.
  CHECK_OK(e_->RunWithAffinity(web_socket, [&]() {
    web_socket->Post(
        io::SourceBuffer(kMessage, sizeof(kMessage), [&]() { sent.Notify(); }));
  }));

  // Post the buffering for the inbound message;
  char inbound[64];
  std::atomic<size_t> bytes_received{0};
  Notification received;  // Note that on a failure this test case can segfault.
  CHECK_OK(e_->RunWithAffinity(web_socket, [&]() {
    web_socket->Post(io::SinkBuffer(inbound, 64, [&](size_t bytes) {
      bytes_received.store(bytes, std::memory_order_release);
      received.Notify();
    }));
  }));

  // Client sends "Hello, world!" to the server
  constexpr char kClientHello[] = "Hello, world!";
  CHECK_OK(
      core::idioms::WriteExactly(s0, {kClientHello, sizeof(kClientHello)}));

  // Wait for the server to receive the inbound message from the client
  received.WaitForNotification();
  EXPECT_EQ(bytes_received.load(std::memory_order_acquire),
            sizeof(kClientHello));
  EXPECT_THAT(inbound, testing::HasSubstr("Hello, world!"));

  // Client reads the response sent by the server
  char client_response[64];
  CHECK_OK(core::idioms::ReadExactly(s0, client_response, sizeof(kMessage)));

  // Wait for the server's send operation completion notification
  sent.WaitForNotification();

  // Verify the response matches what the server posted
  std::string_view response_view(client_response, sizeof(kMessage));
  EXPECT_THAT(response_view, testing::HasSubstr("You are thusly greeted."));
}

}  // namespace
