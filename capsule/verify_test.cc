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

}  // namespace capsule
