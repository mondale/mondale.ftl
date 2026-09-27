#include "io/context.h"
#include "testing/testing.h"

namespace io {

class TestContext : public Context {
 public:
  TestContext() : Context(MonotonicTime::Now()) {}
  void RequestRead(FdHandle h) override {}
  void RequestWrite(FdHandle h) override {}
  void Run(FdHandle h, std::move_only_function<void()> fn) override {}
  ResultOr<FdHandle> Add(std::shared_ptr<IoHandler> h,
                         core::FileDescriptor fd) override {
    return FdHandle(7);
  }
};

TEST(SimpleTest) {
  TestContext tc;
  EXPECT_NE(0, tc.loop_start().nanos());
}

}  // namespace io
