#include "capsule/verify.h"
#include "testing/testing.h"

using testing::HasSubstr;

namespace capsule {

TEST(NoCapsules) {
  CapsuleFile cf;
  cf.capsules.clear();
  EXPECT_THAT(VerifyAtLeastOneCapsule(cf).ToString(),
              HasSubstr("No capsules defined"));
}

TEST(BogusType) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  auto& f = cf.capsules[0].fields[0];
  f.name = "Lolz";
  f.srcloc = "Here";
  f.type = "i9999";
  const auto str = VerifyTypeSoundness(cf).ToString();
  EXPECT_THAT(str, HasSubstr("Here"));
  EXPECT_THAT(str, HasSubstr("i9999"));
  EXPECT_THAT(str, HasSubstr("Unrecognized"));
}

TEST(NameCollisionsWithTypes) {
  CapsuleFile cf;
  cf.capsules.resize(2);
  cf.capsules[0].srcloc = "EvilCapsule";
  cf.capsules[0].name = "u8";
  cf.capsules[1].name = "OkCapsule";
  cf.capsules[0].fields.resize(2);
  cf.capsules[0].fields[0].srcloc = "19";
  cf.capsules[0].fields[0].name = "u32";
  cf.capsules[0].fields[1].srcloc = "20";
  cf.capsules[0].fields[1].name = "OkCapsule";
  const auto str = VerifyNamesDistinctFromTypes(cf).ToString();

  // Capsules can't be named after primitives.
  EXPECT_THAT(str, HasSubstr("EvilCapsule"));
  EXPECT_THAT(str, HasSubstr("u8"));

  // Fields can't be named after primitives.
  EXPECT_THAT(str, HasSubstr("19"));
  EXPECT_THAT(str, HasSubstr("u32"));

  // Fields can't be named after capsules.
  EXPECT_THAT(str, HasSubstr("20"));
  EXPECT_THAT(str, HasSubstr("OkCapsule"));
}

}  // namespace capsule
