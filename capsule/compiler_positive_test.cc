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

}  // namespace
