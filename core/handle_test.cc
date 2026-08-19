#include "base/basic_test.h"
#include "core/handle.h"

using namespace std::string_literals;

namespace {

HANDLE_TYPE(Int64HandleA, int64_t);
HANDLE_TYPE(Int64HandleB, int64_t);

TEST(InvalidValue) {
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),
            Int64HandleA::kInvalid.value());
}

TEST(HandleComparison) {
  auto six = Int64HandleA(6);
  auto seven = Int64HandleA(7);
  auto also_six = Int64HandleB(6);
  auto also_seven = Int64HandleB(7);
  EXPECT_EQ(six, six);
  EXPECT_NE(six, seven);
  EXPECT_EQ(also_six, also_six);
  EXPECT_NE(also_six, also_seven);

  // All of the following should not compile.
  // six == also_six;
  // six++;
  // six + seven;
}

TEST(HandleToString) {
  const auto six = Int64HandleA(6);
  EXPECT_EQ(six.ToString(), "6"s);
  std::stringstream ss;
  ss << six;
  EXPECT_EQ(ss.str(), "6"s);
}

}  // namespace
