#include "core/util.h"
#include "testing/testing.h"

using namespace core;

namespace {

TEST(CleanupBasic) {
  int x = 0;
  {
    auto cleanup = util::MakeCleanup([&x]() { x = 1; });
  }
  EXPECT_EQ(1, x);
}

TEST(CleanupCancel) {
  int x = 0;
  {
    auto cleanup = util::MakeCleanup([&x]() { x = 1; });
    cleanup.Cancel();
  }
  EXPECT_EQ(0, x);
}

int global_x = 0;
void SetXToOne() { global_x = 1; }

TEST(CleanupRawFunctionPointer) {
  global_x = 0;
  {
    auto cleanup = util::MakeCleanup(&SetXToOne);
  }
  EXPECT_EQ(1, global_x);
}

}  // namespace
