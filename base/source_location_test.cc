#include "base/basic_test.h"
#include "base/source_location.h"

using namespace std::string_literals;

namespace {

TEST(SourceLocationBasics) {
  const auto sl = base::SourceLocation::Current();
  EXPECT_EQ(sl.line(), 9);
  EXPECT_EQ(sl.column(), 19);
  EXPECT_EQ(std::string(sl.file()), "base/source_location_test.cc"s);
  EXPECT_EQ(std::string(sl.function()), "Run"s);
}

}  // namespace
