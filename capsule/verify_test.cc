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

}  // namespace capsule
