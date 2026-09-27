#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

#include "net/socket_address.h"
#include "testing/testing.h"

namespace {

using net::SocketAddress;

TEST(SocketAddressTest_IPv4Basic) {
  struct sockaddr_storage ss;
  std::memset(&ss, 0, sizeof(ss));
  auto* sin = reinterpret_cast<struct sockaddr_in*>(&ss);
  sin->sin_family = AF_INET;
  sin->sin_port = htons(8080);
  ASSERT_EQ(1, ::inet_pton(AF_INET, "192.168.1.50", &(sin->sin_addr)));

  SocketAddress addr(ss);
  EXPECT_EQ(addr.IpAddress(), "192.168.1.50");
  EXPECT_EQ(addr.Port(), 8080);
  EXPECT_EQ(addr.ToString(), "192.168.1.50:8080");
}

TEST(SocketAddressTest_IPv6Basic) {
  struct sockaddr_storage ss;
  std::memset(&ss, 0, sizeof(ss));
  auto* sin6 = reinterpret_cast<struct sockaddr_in6*>(&ss);
  sin6->sin6_family = AF_INET6;
  sin6->sin6_port = htons(9090);
  ASSERT_EQ(1, ::inet_pton(AF_INET6, "2001:db8::1", &(sin6->sin6_addr)));

  SocketAddress addr(ss);
  EXPECT_EQ(addr.IpAddress(), "2001:db8::1");
  EXPECT_EQ(addr.Port(), 9090);
  EXPECT_EQ(addr.ToString(), "[2001:db8::1]:9090");
}

TEST(SocketAddressTest_CopyAndMoveSemantics) {
  struct sockaddr_storage ss;
  std::memset(&ss, 0, sizeof(ss));
  auto* sin = reinterpret_cast<struct sockaddr_in*>(&ss);
  sin->sin_family = AF_INET;
  sin->sin_port = htons(1234);
  ASSERT_EQ(1, ::inet_pton(AF_INET, "10.0.0.1", &(sin->sin_addr)));

  SocketAddress addr1(ss);

  // Test copy constructor
  SocketAddress addr2(addr1);
  EXPECT_EQ(addr2.IpAddress(), "10.0.0.1");
  EXPECT_EQ(addr2.Port(), 1234);
  EXPECT_EQ(addr2.ToString(), "10.0.0.1:1234");

  // Test move constructor
  SocketAddress addr3(std::move(addr1));
  EXPECT_EQ(addr3.IpAddress(), "10.0.0.1");
  EXPECT_EQ(addr3.Port(), 1234);
  EXPECT_EQ(addr3.ToString(), "10.0.0.1:1234");
}

TEST(SocketAddressTest_FromStringIPv4) {
  ResultOr<SocketAddress> res = SocketAddress::FromString("127.0.0.1:80");
  ASSERT_TRUE(res.IsOk());
  SocketAddress addr = res.ValueOrDie();
  EXPECT_EQ(addr.IpAddress(), "127.0.0.1");
  EXPECT_EQ(addr.Port(), 80);
  EXPECT_EQ(addr.ToString(), "127.0.0.1:80");
}

TEST(SocketAddressTest_FromStringIPv6Bracketed) {
  ResultOr<SocketAddress> res = SocketAddress::FromString("[::1]:443");
  ASSERT_TRUE(res.IsOk());
  SocketAddress addr = res.ValueOrDie();
  EXPECT_EQ(addr.IpAddress(), "::1");
  EXPECT_EQ(addr.Port(), 443);
  EXPECT_EQ(addr.ToString(), "[::1]:443");
}

TEST(SocketAddressTest_FromStringErrors) {
  // Missing port
  EXPECT_FALSE(SocketAddress::FromString("127.0.0.1").IsOk());

  // Unbracketed IPv6 (ambiguous)
  EXPECT_FALSE(SocketAddress::FromString("2001:db8::1:8080").IsOk());

  // Missing closing bracket
  EXPECT_FALSE(SocketAddress::FromString("[::1:8080").IsOk());

  // Missing separator after bracket
  EXPECT_FALSE(SocketAddress::FromString("[::1]8080").IsOk());

  // Invalid IP format
  EXPECT_FALSE(SocketAddress::FromString("999.999.999.999:80").IsOk());

  // Out of range port
  EXPECT_FALSE(SocketAddress::FromString("127.0.0.1:70000").IsOk());
}

}  // namespace
