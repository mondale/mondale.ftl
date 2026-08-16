#include <stdlib.h>
#include <unistd.h>

#include "base/basic_test.h"
#include "base/process.h"

namespace {

TEST(StackDump) {
  base::DumpStackTrace(STDERR_FILENO);
  ASSERT_TRUE(false);
  char name[] = "StackTraceTestXXXXXX";
  const int fd = mkstemp(name);
  ASSERT_GT(fd, 0);
  base::DumpStackTrace(fd);
  ASSERT_EQ(static_cast<off_t>(0), lseek(fd, 0, SEEK_SET));

  //
}

TEST(SignalHandler) {
  //
}

}  // namespace
