#include "core/syscalls.h"
#include "io/epoller.h"
#include "net/listener.h"
#include "net/socket_address.h"
#include "testing/testing.h"

namespace {

constexpr uint16_t kPort = 1337u;

class ListenerTest : public ::testing::Test {
 protected:
  void OnAccept(core::FileDescriptor fd, net::SocketAddress sa) { ++accepts_; }

  // TODO - factor to a common helper
  bool Await(std::move_only_function<bool()> cond) {
    constexpr Duration kMaxWait = Seconds(1);
    const auto stop = WallTime::Now() + kMaxWait;
    while (WallTime::Now() < stop) {
      if (cond()) return true;
      SleepFor(Milliseconds(1));
    }
    return false;
  }

  std::shared_ptr<io::Epoller> ep_ = io::Epoller::Build(1).ValueOrDie();
  std::shared_ptr<net::Listener> l_ =
      net::Listener::Build(
          ep_.get(), kPort,
          [this](core::FileDescriptor fd, net::SocketAddress sa) {
            OnAccept(std::move(fd), std::move(sa));
          })
          .ValueOrDie();
  int accepts_ = 0;
};

TEST_F(ListenerTest, CanConnect) {
  const auto saddr = strings::Format("[::1]:{}", kPort);
  auto sa = net::SocketAddress::FromString(saddr).ValueOrDie();
  auto sock = core::syscalls::Socket(AF_INET6, SOCK_STREAM, 0).ValueOrDie();
  CHECK_OK(core::syscalls::Connect(sock, sa.AsSockaddr(), sa.socklen()));
  EXPECT_TRUE(Await([&]() { return accepts_ > 0; }));
  EXPECT_EQ(accepts_, 1);
}

}  // namespace
