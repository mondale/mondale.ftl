#include "core/idioms.h"
#include "core/syscalls.h"
#include "io/epoller.h"
#include "io/test_handlers.h"
#include "testing/testing.h"

using testing::IsOk;
using testing::Not;

namespace io {
namespace {

using testing::EagerSwallowHandler;
using testing::Stuff;

class EpollerTest : public ::testing::Test {
 protected:
  std::pair<core::FileDescriptor, core::FileDescriptor> MakeSocketPair() {
    return core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  }

  bool Await(std::move_only_function<bool()> cond) {
    constexpr Duration kMaxWait = Seconds(1);
    const auto stop = WallTime::Now() + kMaxWait;
    while (WallTime::Now() < stop) {
      if (cond()) return true;
      SleepFor(Milliseconds(1));
    }
    return false;
  }
};

TEST_F(EpollerTest, BuildInvalidSilosFails) {
  auto res = Epoller::Build(0);
  EXPECT_THAT(res.result(), Not(IsOk()));

  auto res_negative = Epoller::Build(-1);
  EXPECT_THAT(res_negative.result(), Not(IsOk()));
}

TEST_F(EpollerTest, BuildAndTeardown) {
  auto epoller_res = Epoller::Build(2);
  EXPECT_TRUE(epoller_res.IsOk());
  // Destruction of the unique_ptr here will test clean thread shutdown.
}

TEST_F(EpollerTest, RegisterAndDispatch) {
  auto epoller = std::move(Epoller::Build(2).ValueOrDie());

  Stuff stuff;
  auto handler = std::make_shared<EagerSwallowHandler>(&stuff);

  // Raw socket pair (both blocking by default)
  auto [mine, silo_fd] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();

  // Use the convenience method that sets O_NONBLOCK and registers in one shot
  EXPECT_THAT(epoller->SetNonBlockingAndRegister(handler, std::move(silo_fd)),
              IsOk());

  std::string_view msg = "convenience-test";
  CHECK_OK(core::idioms::WriteExactly(mine, msg));

  EXPECT_TRUE(Await([&]() { return stuff.BytesRead() >= msg.length(); }));
  EXPECT_EQ(stuff.BytesRead(), msg.length());
}

}  // namespace
}  // namespace io
