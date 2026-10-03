#include <arpa/inet.h>

#include "core/syscalls.h"
#include "core/vocabulary.h"
#include "http/http_server.h"
#include "testing/testing.h"

using testing::HasSubstr;

namespace http {

class HttpServerTest : public ::testing::Test {
 protected:
  std::shared_ptr<HttpServer> h_ = HttpServer::Create(1337).ValueOrDie();
};

TEST_F(HttpServerTest, FireUp) {
  // Create client socket
  ResultOr<core::FileDescriptor> sock_res =
      core::syscalls::Socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_TRUE(sock_res.IsOk());
  core::FileDescriptor sock = std::move(sock_res).ValueOrDie();

  // Configure loopback address for port 1337
  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1337);
  ASSERT_EQ(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);

  // Connect to the HTTP server
  Result conn_res = core::syscalls::Connect(
      sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
  ASSERT_TRUE(conn_res.IsOk());

  // Send a GET request for index.html
  std::string request =
      "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: "
      "close\r\n\r\n";
  ResultOr<size_t> write_res =
      core::syscalls::Write(sock, request.data(), request.size());
  ASSERT_TRUE(write_res.IsOk());

  // Read the response
  char buf[4096];
  ResultOr<size_t> recv_res =
      core::syscalls::Recv(sock, buf, sizeof(buf) - 1, 0);
  ASSERT_TRUE(recv_res.IsOk());

  size_t bytes_read = recv_res.ValueOrDie();
  std::string response(buf, bytes_read);

  // Verify a 404 response is returned
  EXPECT_THAT(response, HasSubstr("404"));
}

}  // namespace http
