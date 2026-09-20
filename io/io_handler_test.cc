#include "io/io_handler.h"
#include "testing/testing.h"

namespace io {

class TestIoHandler final : public IoHandler {
 public:
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }
};

TEST(Affininty) {
  TestIoHandler tih;
  tih.SetAffinity(4);
  EXPECT_EQ(tih.GetAffinity(), 4);
}

}  // namespace io
