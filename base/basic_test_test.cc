#include <cassert>
#include <iostream>
#include <memory>

#include "base/basic_test.h"

namespace {

BASIC_TEST(PassingTest) {}

BASIC_TEST(AddFailureTest) {
  ExpectFailure();
  ADD_FAILURE("Oh no!");
}

BASIC_TEST(ExpectEqPassingTest) {
  EXPECT_EQ(7, 7);
  EXPECT_EQ(7.7, 7.7);

  std::string a = "foo";
  std::string b = "foo";
  EXPECT_EQ(a, b);
}

BASIC_TEST(ExpectEqFailingTest) {
  ExpectFailure();
  EXPECT_EQ(7, 8);
}

BASIC_TEST(ExpectNePassingTest) {
  EXPECT_NE(7, 8);
  EXPECT_NE(7.7, 8.8);

  std::string a = "foo";
  std::string b = "bar";
  EXPECT_NE(a, b);
}

BASIC_TEST(ExpectNeFailingTest) {
  ExpectFailure();
  EXPECT_NE(7, 7);
}

BASIC_TEST(ExpectLtPassingTest) { EXPECT_LT(6, 7); }

BASIC_TEST(ExpectLtFailingTest) {
  ExpectFailure();
  EXPECT_LT(6, 6);
}

}  // namespace

int main(int argc, char* argv[]) { return base::testing::RunAllTests(); }
