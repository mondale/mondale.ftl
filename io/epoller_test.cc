#include "core/idioms.h"
#include "core/syscalls.h"
#include "io/epoller.h"
#include "io/test_handlers.h"
#include "testing/testing.h"

using testing::IsOk;
using testing::Not;

namespace io {
namespace {

using testing::CarlyHandler;
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

TEST_F(EpollerTest, CarlyHandlerAsyncRunDispatch) {
  Stuff stuff;
  auto epoller = std::move(Epoller::Build(1).ValueOrDie());
  auto handler = std::make_shared<CarlyHandler>(&stuff);

  auto [mine, silo_fd] =
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie();
  EXPECT_THAT(epoller->SetNonBlockingAndRegister(handler, std::move(silo_fd)),
              IsOk());

  // First write: Triggers the `if (maybe_)` branch, reads data, sets maybe_ =
  // false.
  std::string_view msg1 = "first-call";
  CHECK_OK(core::idioms::WriteExactly(mine, msg1));

  // Wait for the first read upcall to process
  EXPECT_TRUE(Await([&]() { return stuff.Reads() >= 1; }));

  // Second write: Hits the `else` branch, invokes c->Run(...) which schedules
  // CallMeMaybe, resets maybe_ = true, and re-arms read interest.
  std::string_view msg2 = "second-call-maybe";
  CHECK_OK(core::idioms::WriteExactly(mine, msg2));

  // Verify that the deferred Context::Run callback executed and processed the
  // second read
  EXPECT_TRUE(Await([&]() { return stuff.Reads() >= 2; }));
  EXPECT_GE(stuff.BytesRead(), msg1.length());
}

}  // namespace
}  // namespace io
