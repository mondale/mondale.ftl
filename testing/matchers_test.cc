#include <sstream>

// hack, but that's life
namespace {
bool IsOk(int x) { return x == 7; }
}  // namespace

#include "testing/matchers.h"
#include "testing/test.h"

namespace {

TEST(HasSubstr) {
  auto m = testing::HasSubstr("friends");
  std::stringstream ss;
  m.DescribeTo(ss);
  EXPECT_EQ(ss.str(), "contains substring [friends]");

  const auto yup = m.Match("you don't have any friends");
  EXPECT_TRUE(yup.matched);
  EXPECT_TRUE(yup.explanation.empty());

  const auto nope = m.Match("gollum");
  EXPECT_FALSE(nope.matched);
  EXPECT_EQ(nope.explanation, "gollum");
}

TEST(IsOk) {
  EXPECT_THAT(7, testing::IsOk());
  EXPECT_THAT(8, testing::Not(testing::IsOk()));
}

}  // namespace
