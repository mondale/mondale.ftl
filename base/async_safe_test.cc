#include "base/async_safe.h"
#include "base/basic_test.h"

using namespace base::async_safe;

namespace {

TEST(StrLenEmpty) {
  EXPECT_EQ(0u, StrLen(""));
}

TEST(StrLenSimple) {
  EXPECT_EQ(5u, StrLen("hello"));
}

TEST(StrLenEmbeddedNull) {
  char s[] = {'a', '\0', 'b', '\0'};
  EXPECT_EQ(1u, StrLen(s));
}

TEST(StrLenLong) {
  // Create a buffer of 1000 'x' characters followed by a null terminator.
  char buf[1001];
  for (int i = 0; i < 1000; ++i) buf[i] = 'x';
  buf[1000] = '\0';
  EXPECT_EQ(1000u, StrLen(buf));
}

}  // namespace
