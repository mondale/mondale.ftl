#include "capsule/compiler_smoke_test.capsule.h"
#include "testing/testing.h"

using testing::HasSubstr;

namespace {

TEST(DoesItWork) {
  test_ns::FullFeatureM m;  // I guess it works if this compiles.
}

TEST(ToStringLittlePair) {
  test_ns::LittlePairM m;
  m.crc = 4;
  m.name = "Jolly";
  EXPECT_THAT(m.ToString(), HasSubstr("crc: 4"));
  EXPECT_THAT(m.ToString(), HasSubstr("name: \"Jolly\""));
}

TEST(ToStringPileOfPrimitives) {
  test_ns::PileOfPrimitivesM m;
  m.u_eight = 8;
  m.i_eight = -8;
  m.u_sixteen = 16;
  m.i_sixteen = -16;
  m.u_thirtytwo = 32;
  m.i_thirtytwo = -32;
  m.u_sixtyfour = 64;
  m.i_sixtyfour = -64;

  std::string s = m.ToString();
  EXPECT_THAT(s, HasSubstr("u_eight: 8"));
  EXPECT_THAT(s, HasSubstr("i_eight: -8"));
  EXPECT_THAT(s, HasSubstr("u_sixteen: 16"));
  EXPECT_THAT(s, HasSubstr("i_sixteen: -16"));
  EXPECT_THAT(s, HasSubstr("u_thirtytwo: 32"));
  EXPECT_THAT(s, HasSubstr("i_thirtytwo: -32"));
  EXPECT_THAT(s, HasSubstr("u_sixtyfour: 64"));
  EXPECT_THAT(s, HasSubstr("i_sixtyfour: -64"));
}

TEST(ToStringFullFeature) {
  test_ns::FullFeatureM m;
  m.id = 100;
  m.code = 7;
  m.enabled = true;
  m.name = "Elizabeth";
  m.items = {"sword", "shield"};
  m.nested.u_thirtytwo = 99;

  test_ns::LittlePairM lp;
  lp.crc = 123;
  lp.name = "TestPair";
  m.little_stuff.push_back(lp);

  m.count = 10;

  std::string s = m.ToString();
  EXPECT_THAT(s, HasSubstr("id: 100"));
  EXPECT_THAT(s, HasSubstr("code: 7"));
  EXPECT_THAT(s, HasSubstr("enabled: 1"));
  EXPECT_THAT(s, HasSubstr("name: \"Elizabeth\""));
  EXPECT_THAT(s, HasSubstr("sword"));
  EXPECT_THAT(s, HasSubstr("shield"));
  EXPECT_THAT(s, HasSubstr("u_thirtytwo: 99"));
  EXPECT_THAT(s, HasSubstr("crc: 123"));
  EXPECT_THAT(s, HasSubstr("name: \"TestPair\""));
  EXPECT_THAT(s, HasSubstr("count: 10"));
}

}  // namespace
