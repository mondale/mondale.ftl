#include <array>
#include <list>
#include <vector>

#include "base/basic_test.h"
#include "core/strings.h"

using namespace std::string_literals;

namespace core::strings {

TEST(FormatHexPadding) {
  const auto s = Format("0x{:016x}", 32ull);
  EXPECT_EQ(s, "0x0000000000000020"s);
}

TEST(FormatMultipleArguments) {
  const auto s = Format("{} {} {:.2f}", "Test", 42, 3.14159);
  EXPECT_EQ(s, "Test 42 3.14"s);
}

TEST(FormatPositionalArguments) {
  const auto s = Format("{1} then {0}", "first", "second");
  EXPECT_EQ(s, "second then first"s);
}

TEST(JoinVectorOfString) {
  std::vector<std::string> v = {"apple", "banana", "cherry"};
  EXPECT_EQ(Join(v, ", "), "apple, banana, cherry"s);
}

TEST(JoinListOfConstPtr) {
  std::list<const char*> l = {"one", "two", "three"};
  EXPECT_EQ(Join(l, "-"), "one-two-three"s);
}

TEST(JoinArrayOfStringView) {
  std::array<std::string_view, 3> a = {"foo", "bar", "baz"};
  EXPECT_EQ(Join(a, "::"), "foo::bar::baz"s);
}

TEST(JoinSingleElement) {
  std::vector<std::string> v = {"solo"};
  EXPECT_EQ(Join(v, ", "), "solo"s);
}

TEST(JoinEmptyContainer) {
  std::vector<std::string> v;
  EXPECT_EQ(Join(v, ", "), ""s);
}

TEST(JoinEmptyDelimiter) {
  std::vector<const char*> v = {"a", "b", "c"};
  EXPECT_EQ(Join(v, ""), "abc"s);
}

TEST(JoinBracerList) { EXPECT_EQ(Join({"a", "b", "cd"}, ", "), "a, b, cd"s); }

}  // namespace core::strings
