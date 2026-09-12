#include <array>
#include <list>
#include <vector>

#include "core/strings.h"
#include "testing/testing.h"

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

TEST(ParseAsValidIntegers) {
  auto val_int = ParseAs<int>("42").ValueOrDie();
  EXPECT_EQ(val_int, 42);

  uint64_t val_u64 = ParseAs<uint64_t>("18446744073709551615").ValueOrDie();
  EXPECT_EQ(val_u64, 18446744073709551615ul);

  auto val_hex = ParseAs<int>("1a", 16).ValueOrDie();
  EXPECT_EQ(val_hex, 26);
}

TEST(ParseAsValidFloats) {
  const auto val_double = ParseAs<double>("3.14159").ValueOrDie();
  // Basic test has no double support.
  EXPECT_GT(val_double, 3.14);
  EXPECT_LT(val_double, 3.142);
}

TEST(ParseAsInvalidInput) {
  auto res1 = ParseAs<int>("42abc");
  EXPECT_FALSE(res1.ok());  // Trailing unparsed characters

  auto res2 = ParseAs<int>("not_a_number");
  EXPECT_FALSE(res2.ok());  // Invalid format

  auto res3 = ParseAs<uint8_t>("256");
  EXPECT_FALSE(res3.ok());  // Out of range
}

TEST(ParseAsStringViewPassThrough) {
  auto sv = ParseAs<std::string_view>("hello").ValueOrDie();
  EXPECT_EQ(sv, std::string_view("hello"));
}

TEST(SplitStandardDelimiter) {
  std::vector<std::string_view> tokens;
  for (std::string_view token : Split("apple,banana,cherry", ",")) {
    tokens.push_back(token);
  }
  std::vector<std::string_view> expected = {"apple", "banana", "cherry"};
  for (int i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(tokens[i], expected[i]);
  }
}

TEST(SplitConsecutiveAndEdgeDelimiters) {
  std::vector<std::string_view> tokens;
  for (std::string_view token : Split(",a,,b,", ",")) {
    tokens.push_back(token);
  }
  std::vector<std::string_view> expected = {"", "a", "", "b", ""};
  for (int i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(tokens[i], expected[i]);
  }
}

TEST(SplitEmptyDelimiter) {
  std::vector<std::string_view> tokens;
  for (std::string_view token : Split("abc", "")) {
    tokens.push_back(token);
  }
  std::vector<std::string_view> expected = {"a", "b", "c"};
  for (int i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(tokens[i], expected[i]);
  }
}

TEST(SplitMultiCharacterDelimiter) {
  std::vector<std::string_view> tokens;
  for (std::string_view token : Split("one::two::three", "::")) {
    tokens.push_back(token);
  }
  std::vector<std::string_view> expected = {"one", "two", "three"};
  for (int i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(tokens[i], expected[i]);
  }
}

TEST(EscapeStringTest_BasicEscapes) {
  EXPECT_EQ(EscapeString("Hello \"World\""), "Hello \\\"World\\\"");
  EXPECT_EQ(EscapeString("Line1\nLine2\tTabbed"), "Line1\\nLine2\\tTabbed");
  EXPECT_EQ(EscapeString("Backslash: \\"), "Backslash: \\\\");
}

TEST(EscapeStringTest_ValidUtf8Preserved) {
  // Valid multi-byte UTF-8 sequences (e.g., "Hello, ??!") should remain
  // untouched
  EXPECT_EQ(EscapeString("Hello, ?\?!"), "Hello, ?\?!");
}

TEST(EscapeStringTest_InvalidUtf8Escaped) {
  // Truncated multi-byte byte sequence 0xE6 0x97 (missing third byte)
  std::string_view bad_utf8(
      "\xE6"
      "\x97",
      2);
  EXPECT_EQ(EscapeString(bad_utf8), "\\xe6\\x97");
}

TEST(UnEscapeStringTest_Success) {
  auto res = UnescapeString("Hello \\\"World\\\"\\n\\t\\\\");
  ASSERT_TRUE(res.IsOk());
  EXPECT_EQ(res.ValueOrDie(), "Hello \"World\"\n\t\\");

  auto hex_res = UnescapeString("\\x48\\x65\\x6c\\x6c\\x6f");
  ASSERT_TRUE(hex_res.IsOk());
  EXPECT_EQ(hex_res.ValueOrDie(), "Hello");
}

TEST(UnEscapeStringTest_InvalidEscapesFail) {
  EXPECT_FALSE(UnescapeString("Trailing \\").IsOk());
  EXPECT_FALSE(UnescapeString("Invalid \\z escape").IsOk());
  EXPECT_FALSE(UnescapeString("Incomplete \\x1").IsOk());
}

}  // namespace core::strings
