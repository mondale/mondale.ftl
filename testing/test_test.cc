#include <cassert>
#include <iostream>
#include <memory>

#include "testing/test.h"

namespace {

TEST(PassingTest) {}

TEST(AddFailureTest) {
  ExpectFailure();
  ADD_FAILURE("Oh no!");
}

TEST(ExpectTruePassingTest) {
  EXPECT_TRUE(7 == 7);
  EXPECT_TRUE(true);
}

TEST(ExpectTrueFalingTest) {
  ExpectFailure();
  EXPECT_TRUE(7 == 8);
}

TEST(ExpectFalsePassingTest) {
  EXPECT_FALSE(7 == 8);
  EXPECT_FALSE(false);
}

TEST(ExpectFalseFailingTest) {
  ExpectFailure();
  EXPECT_FALSE(true);
}

TEST(ExpectEqPassingTest) {
  EXPECT_EQ(7, 7);
  EXPECT_EQ(7.7, 7.7);

  std::string a = "foo";
  std::string b = "foo";
  EXPECT_EQ(a, b);
}

TEST(ExpectEqFailingTest) {
  ExpectFailure();
  EXPECT_EQ(7, 8);
}

TEST(ExpectNePassingTest) {
  EXPECT_NE(7, 8);
  EXPECT_NE(7.7, 8.8);

  std::string a = "foo";
  std::string b = "bar";
  EXPECT_NE(a, b);
}

TEST(ExpectNeFailingTest) {
  ExpectFailure();
  EXPECT_NE(7, 7);
}

TEST(ExpectLtPassingTest) { EXPECT_LT(6, 7); }

TEST(ExpectLtFailingTest) {
  ExpectFailure();
  EXPECT_LT(6, 6);
}

TEST(ExpectLePassingTest) {
  EXPECT_LE(6, 6);
  EXPECT_LE(6, 7);
}

TEST(ExpectLeFailingTest) {
  ExpectFailure();
  EXPECT_LE(87, 7);
}

TEST(ExpectGtPassingTest) { EXPECT_GT(7, 6); }

TEST(ExpectGtFailingTest) {
  ExpectFailure();
  EXPECT_GT(6, 6);
}

TEST(ExpectGePassingTest) {
  EXPECT_GE(7, 6);
  EXPECT_GE(6, 6);
}

TEST(ExpectGeFailingTest) {
  ExpectFailure();
  EXPECT_GE(6, 87);
}

TEST(AssertTruePassingTest) { ASSERT_TRUE(true); }

TEST(AssertTrueFailingTest) {
  ExpectFailure();
  ASSERT_TRUE(false);
}

TEST(AssertFalsePassingTest) { ASSERT_FALSE(false); }

TEST(AssertFalseFailingTest) {
  ExpectFailure();
  ASSERT_FALSE(true);
}

TEST(AssertEqPassingTest) {
  ASSERT_EQ(7, 7);
  ASSERT_EQ(7.7, 7.7);

  std::string a = "foo";
  std::string b = "foo";
  ASSERT_EQ(a, b);
}

TEST(AssertEqFailingTest) {
  ExpectFailure();
  ASSERT_EQ(7, 8);
}

TEST(AssertNePassingTest) {
  ASSERT_NE(7, 8);
  ASSERT_NE(7.7, 8.8);

  std::string a = "foo";
  std::string b = "bar";
  ASSERT_NE(a, b);
}

TEST(AssertNeFailingTest) {
  ExpectFailure();
  ASSERT_NE(7, 7);
}

TEST(AssertLtPassingTest) { ASSERT_LT(6, 7); }

TEST(AssertLtFailingTest) {
  ExpectFailure();
  ASSERT_LT(6, 6);
}

TEST(AssertLePassingTest) {
  ASSERT_LE(6, 6);
  ASSERT_LE(6, 7);
}

TEST(AssertLeFailingTest) {
  ExpectFailure();
  ASSERT_LE(87, 7);
}

TEST(AssertGtPassingTest) { ASSERT_GT(7, 6); }

TEST(AssertGtFailingTest) {
  ExpectFailure();
  ASSERT_GT(6, 6);
}

TEST(AssertGePassingTest) {
  ASSERT_GE(7, 6);
  ASSERT_GE(6, 6);
}

TEST(AssertGeFailingTest) {
  ExpectFailure();
  ASSERT_GE(6, 87);
}

}  // namespace
