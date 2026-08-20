#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <array>

#include "base/basic_test.h"
#include "core/file_descriptor.h"

using namespace core;

namespace {

bool IsFdOpen(int fd) {
  // F_GETFD returns the file descriptor flags if valid
  if (fcntl(fd, F_GETFD) == -1) {
    if (errno == EBADF) {
      return false;  // The file descriptor is definitely closed
    }
  }
  return true;  // The file descriptor is open
}

TEST(BasicLifetime) {
  std::array<char, 7> buf;
  memcpy(buf.data(), "XXXXXX", 7);
  const int raw = mkstemp(&buf[0]);
  ASSERT_GT(raw, 0);
  FileDescriptor fd(raw);
  EXPECT_EQ(raw, fd.fd());

  auto fd2 = std::move(fd);
  EXPECT_EQ(raw, fd2.fd());
  EXPECT_TRUE(IsFdOpen(raw));
  {
    auto fd3(std::move(fd2));
    EXPECT_EQ(raw, fd3.fd());
  }
  EXPECT_FALSE(IsFdOpen(raw));
}

}  // namespace
