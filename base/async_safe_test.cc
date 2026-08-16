#include "base/async_safe.h"
#include "base/basic_test.h"

using namespace base::async_safe;

namespace {

template <typename INT>
size_t Size(INT x) {
  return static_cast<size_t>(x);
}

TEST(StrLenEmpty) { EXPECT_EQ(Size(0), StrLen("")); }

TEST(StrLenSimple) { EXPECT_EQ(Size(5), StrLen("hello")); }

TEST(StrLenEmbeddedNull) {
  char s[] = {'a', '\0', 'b', '\0'};
  EXPECT_EQ(Size(1), StrLen(s));
}

TEST(StrLenLong) {
  // Create a buffer of 1000 'x' characters followed by a null terminator.
  char buf[1001];
  for (int i = 0; i < 1000; ++i) buf[i] = 'x';
  buf[1000] = '\0';
  EXPECT_EQ(Size(1000), StrLen(buf));
}

}  // namespace
