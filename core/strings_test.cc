#include "base/basic_test.h"
#include "core/strings.h"

namespace core::strings {

TEST(FormatHexPadding) {
  const auto s = Format("0x{:016x}", 32ull);
  EXPECT_EQ(s, std::string("0x0000000000000020"));
}

TEST(FormatMultipleArguments) {
  const auto s = Format("{} {} {:.2f}", "Test", 42, 3.14159);
  EXPECT_EQ(s, std::string("Test 42 3.14"));
}

TEST(FormatPositionalArguments) {
  const auto s = Format("{1} then {0}", "first", "second");
  EXPECT_EQ(s, std::string("second then first"));
}

}  // namespace core::strings
