#include <string>

#include "testing/compare.h"
#include "testing/testing.h"

using testing::internal::Compare;

namespace {

TEST(EqSameTypes) {
  EXPECT_TRUE(Compare::Eq(7, 7));
  EXPECT_TRUE(Compare::Eq(7u, 7u));
  EXPECT_FALSE(Compare::Eq(7.7, 7.7));
  EXPECT_TRUE(Compare::Eq("Foo", "Foo"));

  std::string s1 = "Bar";
  std::string s2 = "Bar";
  EXPECT_TRUE(Compare::Eq(s1, s2));

  EXPECT_FALSE(Compare::Eq(7, 8));
  EXPECT_FALSE(Compare::Eq(7u, 8u));
  EXPECT_FALSE(Compare::Eq(7.7, 7.8));
  EXPECT_FALSE(Compare::Eq("Foo", "Bar"));

  std::string s3 = "Foo";
  std::string s4 = "Bar";
  EXPECT_FALSE(Compare::Eq(s3, s4));
}

TEST(EqDiffTypes) {
  std::string s1 = "Bar";
  std::string s2 = "Bar";
  EXPECT_TRUE(Compare::Eq(7, 7u));
  EXPECT_TRUE(Compare::Eq(7u, 7));
  EXPECT_TRUE(Compare::Eq(7, 7.0));
  EXPECT_TRUE(Compare::Eq(s1, "Bar"));
  EXPECT_TRUE(Compare::Eq("Bar", s2));

  EXPECT_FALSE(Compare::Eq(7, 8u));
  EXPECT_FALSE(Compare::Eq(7u, 8));
  EXPECT_FALSE(Compare::Eq(7u, 8.0));
  EXPECT_FALSE(Compare::Eq(static_cast<float>(7.7), static_cast<double>(7.8)));
  EXPECT_FALSE(Compare::Eq("Foo", s2));

  std::string s3 = "Foo";
  std::string s4 = "Bar";
  EXPECT_FALSE(Compare::Eq(s3, "Bar"));
}

TEST(CursoryNe) { EXPECT_TRUE(Compare::Ne(7, 8)); }

TEST(Lt) {
  EXPECT_TRUE(Compare::Lt(6, 7));
  EXPECT_TRUE(Compare::Lt(6, 7.7));
  EXPECT_TRUE(Compare::Lt(6, 7u));
  // Should work for lexigraphic comparison even if we frown on that here.
  EXPECT_TRUE(Compare::Lt(std::string("Bar"), std::string("Foo")));
  EXPECT_FALSE(Compare::Lt(7, 6));
  EXPECT_FALSE(Compare::Lt(7, 5.5));
  EXPECT_FALSE(Compare::Lt(7u, 5));
}

TEST(CursoryGe) { EXPECT_TRUE(Compare::Ge(8, 8)); }

TEST(Le) {
  EXPECT_TRUE(Compare::Le(6, 7));
  EXPECT_TRUE(Compare::Le(6, 6));
  EXPECT_TRUE(Compare::Le(6, 7.7));
  EXPECT_TRUE(Compare::Le(6, 7u));
  // Should work for lexigraphic comparison even if we frown on that here.
  EXPECT_TRUE(Compare::Le(std::string("Bar"), std::string("Foo")));
  EXPECT_FALSE(Compare::Le(7, 6));
  EXPECT_FALSE(Compare::Le(7, 5.5));
  EXPECT_FALSE(Compare::Le(7u, 5));
}

}  // namespace
