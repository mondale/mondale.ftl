#include "base/basic_test.h"
#include "base/rawlog.h"
#include "core/syscalls.h"

using namespace core;

namespace {

TEST(Open) {
  auto result =
      syscalls::Open("/does/not/exist/probably", 0, O_RDONLY).result();
  EXPECT_EQ(BaseCode::kEnoent, result.base_code());
}

}  // namespace
