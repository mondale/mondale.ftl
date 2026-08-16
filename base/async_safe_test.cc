#include <sys/types.h>
#include <unistd.h>

#include "base/async_safe.h"
#include "base/basic_test.h"

namespace {

TEST(StrLen) {
  EXPECT_EQ(size_t{0}, base::async_safe::StrLen(""));
  EXPECT_EQ(size_t{1}, base::async_safe::StrLen("a"));
  EXPECT_EQ(size_t{5}, base::async_safe::StrLen("hello"));
  EXPECT_EQ(size_t{11}, base::async_safe::StrLen("hello world"));
}

TEST(MemCopy) {
  const char src[] = "async_safe";
  char dst[11] = {0};

  base::async_safe::MemCopy(dst, src, sizeof(src));
  EXPECT_TRUE(base::async_safe::StrEqualN(src, dst, sizeof(src)));
}

TEST(StrEqualN) {
  EXPECT_TRUE(base::async_safe::StrEqualN("hello", "hello", 5));
  EXPECT_TRUE(base::async_safe::StrEqualN("hello", "helXX", 3));
  EXPECT_FALSE(base::async_safe::StrEqualN("hello", "helXX", 5));
  EXPECT_TRUE(base::async_safe::StrEqualN("abc", "xyz", 0));
}

TEST(WriteAll) {
  int pipe_fds[2];
  ASSERT_EQ(0, pipe(pipe_fds));

  const char message[] = "test payload";
  const size_t len = sizeof(message) - 1;

  EXPECT_TRUE(base::async_safe::WriteAll(pipe_fds[1], message, len));

  char buf[32] = {0};
  ssize_t bytes_read = read(pipe_fds[0], buf, sizeof(buf));
  EXPECT_EQ(static_cast<ssize_t>(len), bytes_read);
  EXPECT_TRUE(base::async_safe::StrEqualN(message, buf, len));

  close(pipe_fds[0]);
  close(pipe_fds[1]);
}

TEST(EnumerateThreads) {
  int tids[16];
  int count = base::async_safe::EnumerateThreads(tids, 16);

  EXPECT_GT(count, 0);
  EXPECT_LE(count, 16);

  // Verify the current thread ID is found in the returned thread list
  pid_t current_tid = gettid();
  bool found_self = false;
  for (int i = 0; i < count; ++i) {
    if (tids[i] == static_cast<int>(current_tid)) {
      found_self = true;
      break;
    }
  }
  EXPECT_TRUE(found_self);
}

TEST(ParseHex) {
  uint64_t val = 0;

  // Standard lower and upper case hex parsing
  const char* str1 = "1a2B3c";
  const char* p1 = str1;
  const char* end1 = str1 + 6;
  EXPECT_TRUE(base::async_safe::ParseHex(&p1, end1, &val));
  EXPECT_EQ(static_cast<uint64_t>(0x1a2b3c), val);
  EXPECT_EQ(end1, p1);

  // Partial parsing stopping at first non-hex digit
  const char* str2 = "ff_tail";
  const char* p2 = str2;
  const char* end2 = str2 + 7;
  EXPECT_TRUE(base::async_safe::ParseHex(&p2, end2, &val));
  EXPECT_EQ(static_cast<uint64_t>(0xff), val);
  EXPECT_EQ(str2 + 2, p2);

  // Returns false when no hex digits are present
  const char* str3 = "xyz";
  const char* p3 = str3;
  const char* end3 = str3 + 3;
  EXPECT_FALSE(base::async_safe::ParseHex(&p3, end3, &val));
  EXPECT_EQ(str3, p3);
}

TEST(Writer) {
  int pipe_fds[2];
  ASSERT_EQ(0, pipe(pipe_fds));

  {
    base::async_safe::Writer writer(pipe_fds[1]);
    writer.Str("num=");
    writer.Dec(-12345);
    writer.Char(' ');
    writer.Bytes("hex=", 4);
    writer.Hex64(0xabc);
    // Flushes automatically on destruction, but calling Flush explicitly as
    // well
    writer.Flush();
  }

  char buf[64] = {0};
  ssize_t bytes_read = read(pipe_fds[0], buf, sizeof(buf) - 1);
  EXPECT_GT(bytes_read, static_cast<ssize_t>(0));

  close(pipe_fds[0]);
  close(pipe_fds[1]);
}

}  // namespace
