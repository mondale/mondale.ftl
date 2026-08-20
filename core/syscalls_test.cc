#include "base/basic_test.h"
#include "base/rawlog.h"
#include "core/syscalls.h"

using namespace core;

namespace {

TEST(FStatAndRead) {
  auto fd = syscalls::Open("/tmp", O_TMPFILE | O_RDWR, 0).ValueOrDie();
  auto sb = syscalls::FStat(fd).ValueOrDie();
  EXPECT_EQ(static_cast<int>(sb.st_size), 0);
  EXPECT_EQ(size_t{0}, syscalls::Read(fd, nullptr, 100).ValueOrDie());  // eof
}

TEST(Open) {
  auto result =
      syscalls::Open("/does/not/exist/probably", O_RDONLY, 0).result();
  EXPECT_EQ(BaseCode::kEnoent, result.base_code());
}

}  // namespace
