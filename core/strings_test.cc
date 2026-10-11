#include <array>
#include <list>
#include <vector>

#include "core/strings.h"
#include "testing/testing.h"

using namespace std::string_literals;
using testing::HasSubstr;

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

TEST(HexdumpTest_EmptyInput) {
  std::string result = Hexdump("", 0);
  EXPECT_EQ(result, "00000000\n");
}

TEST(HexdumpTest_ShortInput) {
  const char* data = "Hello";
  std::string result = Hexdump(data, 5);

  EXPECT_THAT(result, HasSubstr("00000000  48 65 6c 6c 6f"));
  EXPECT_THAT(result, HasSubstr("|Hello|"));
  EXPECT_THAT(result, HasSubstr("00000005\n"));
}

TEST(HexdumpTest_ExactlySixteenBytes) {
  const char* data = "0123456789abcdef";
  std::string result = Hexdump(data, 16);

  EXPECT_THAT(result, HasSubstr("00000000  30 31 32 33 34 35 36 37  38 39 61 "
                                "62 63 64 65 66  |0123456789abcdef|"));
  EXPECT_THAT(result, HasSubstr("00000010\n"));
}

TEST(HexdumpTest_NonPrintableCharacters) {
  char data[] = {0x00, 0x01, 0x1F, 0x7F, (char)0xFF};
  std::string result = Hexdump(data, 5);

  EXPECT_THAT(result, HasSubstr("00 01 1f 7f ff"));
  EXPECT_THAT(result, HasSubstr("|.....|"));
}

TEST(HexdumpTest_MultiRowInput) {
  // 20 bytes spans across two rows (16 bytes + 4 bytes)
  const char* data = "ABCDEFGHIJKLMNOPQRST";
  std::string result = Hexdump(data, 20);

  EXPECT_THAT(result, HasSubstr("00000000  41 42 43 44 45 46 47 48  49 4a 4b "
                                "4c 4d 4e 4f 50  |ABCDEFGHIJKLMNOP|"));
  EXPECT_THAT(result, HasSubstr("00000010  51 52 53 54                         "
                                "              |QRST|"));
  EXPECT_THAT(result, HasSubstr("00000014\n"));
}

TEST(StartsWithTest_BasicMatches) {
  EXPECT_TRUE(StartsWith("hello world", "hello"));
  EXPECT_TRUE(StartsWith("hello world", "h"));
  EXPECT_TRUE(StartsWith("hello world", "hello world"));
  EXPECT_TRUE(StartsWith("hello world", ""));
  EXPECT_TRUE(StartsWith("", ""));
}

TEST(StartsWithTest_Mismatches) {
  EXPECT_FALSE(StartsWith("hello world", "world"));
  EXPECT_FALSE(StartsWith("hello world", "hello world extra"));
  EXPECT_FALSE(StartsWith("", "a"));
}

TEST(StartsWithTest_MixedStringTypes) {
  std::string s = "foobar";
  std::string_view sv = "foo";
  const char* raw = "foobar";

  EXPECT_TRUE(StartsWith(s, sv));
  EXPECT_TRUE(StartsWith(s, "foo"));
  EXPECT_TRUE(StartsWith(sv, "foo"));
  EXPECT_TRUE(StartsWith(raw, sv));
  EXPECT_TRUE(StartsWith(s, s));
}

using base::Days;
using base::Duration;
using base::Hours;
using base::Microseconds;
using base::Milliseconds;
using base::Minutes;
using base::Nanoseconds;
using base::Seconds;
using core::strings::HumanReadable;

TEST(TimeFormattingTest_ZeroDuration) {
  EXPECT_EQ(HumanReadable(Duration::Zero()), "0ns");
  EXPECT_EQ(HumanReadable(Nanoseconds(0)), "0ns");
}

TEST(TimeFormattingTest_SingleSmallUnit) {
  EXPECT_EQ(HumanReadable(Nanoseconds(8)), "8ns");
  EXPECT_EQ(HumanReadable(Microseconds(5)), "5us");
  EXPECT_EQ(HumanReadable(Milliseconds(42)), "42ms");
  EXPECT_EQ(HumanReadable(Seconds(15)), "15s");
}

TEST(TimeFormattingTest_ExactMultipleUnits) {
  Duration d = Days(3) + Hours(18) + Minutes(22) + Seconds(14);
  EXPECT_EQ(HumanReadable(d), "3d 18h 22m 14s");
}

TEST(TimeFormattingTest_YearsUsedOnlyWhenNonzero) {
  // Less than a year: should NOT include years
  Duration d_short = Days(364) + Hours(23);
  EXPECT_EQ(HumanReadable(d_short), "364d 23h");

  // Exactly or more than a year: should include years
  Duration d_with_years = Days(365) + Hours(5);
  // 365 days = 1y (approx year definition or handled via division)
  // Let's check how many nanoseconds are in 1y: 31536000ULL seconds
  // 365 days * 24 * 3600 = 31536000s, which matches kUnits[0].
  EXPECT_EQ(HumanReadable(d_with_years), "1y 5h");
}

TEST(TimeFormattingTest_MaxFourSignificantUnits) {
  // 5 units provided: y, d, h, m, s -> should truncate or round to top 4 (y, d,
  // h, m) Let's test with hours, minutes, seconds, milliseconds, microseconds
  Duration d = Hours(2) + Minutes(30) + Seconds(45) + Milliseconds(500) +
               Microseconds(123);
  // Top 4 are hours, minutes, seconds, milliseconds
  EXPECT_EQ(HumanReadable(d), "2h 30m 45s 500ms");
}

TEST(TimeFormattingTest_RoundingUpAndDown) {
  Duration d_round_up = Hours(1) + Minutes(1) + Seconds(1) + Milliseconds(500) +
                        Microseconds(600);
  EXPECT_EQ(HumanReadable(d_round_up), "1h 1m 1s 501ms");

  Duration d_round_down = Hours(1) + Minutes(1) + Seconds(1) +
                          Milliseconds(500) + Microseconds(400);
  EXPECT_EQ(HumanReadable(d_round_down), "1h 1m 1s 500ms");
}

TEST(TimeFormattingTest_NegativeDurations) {
  EXPECT_EQ(HumanReadable(Seconds(-5)), "-5s");
  EXPECT_EQ(HumanReadable((Hours(-1) + Minutes(-30))), "-1h 30m");
}

}  // namespace core::strings
