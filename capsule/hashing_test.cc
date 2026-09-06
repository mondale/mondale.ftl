#include "capsule/hashing.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

using testing::HasSubstr;
using testing::IsOk;

namespace {

class UhhFixture : public ::testing::Test {
 protected:
  static constexpr int kCapsules = 3;
  static constexpr int kFields = 3;

  UhhFixture() {
    cf_.capsules.resize(kCapsules);
    for (int i = 0; i < kCapsules; ++i) {
      cf_.capsules[i].fields.resize(kFields);
      for (int j = 0; j < kFields; ++j) {
        auto& f = cf_.capsules[i].fields[j];
        f.name = strings::Format("field_{}_{}", i, j);
        f.srcloc = strings::Format("cap{}_field{}", i, j);
        f.attributes.push_back({"default", "42"});
      }
    }
  }

  void AssertAllFieldsHaveHashes() {
    cf_.capsules.resize(kCapsules);
    for (int i = 0; i < kCapsules; ++i) {
      cf_.capsules[i].fields.resize(kFields);
      for (int j = 0; j < kFields; ++j) {
        auto& f = cf_.capsules[i].fields[j];
        EXPECT_GT(f.hashes.size(), 0);
      }
    }
  }
  capsule::CapsuleFile cf_;
};

TEST_F(UhhFixture, NoShennanigansIsOk) {
  EXPECT_THAT(ComputeAndValidateHashes(&cf_), IsOk());
  AssertAllFieldsHaveHashes();
}

TEST_F(UhhFixture, SameNameInTwoDifferentCapsules) {
  cf_.capsules[0].fields[0].name = cf_.capsules[1].fields[0].name;
  EXPECT_THAT(ComputeAndValidateHashes(&cf_), IsOk());
  AssertAllFieldsHaveHashes();
}

TEST_F(UhhFixture, SameNameInSameCapsules) {
  cf_.capsules[0].fields[0].name = cf_.capsules[0].fields[1].name;
  EXPECT_THAT(ComputeAndValidateHashes(&cf_).ToString(),
              HasSubstr(cf_.capsules[0].fields[0].name));
}

TEST_F(UhhFixture, NonCollidingAliases) {
  cf_.capsules[0].fields[0].attributes.push_back({"former_name", "donkey"});
  cf_.capsules[0].fields[0].attributes.push_back({"former_name", "ape"});
  cf_.capsules[0].fields[0].attributes.push_back({"default", "ape"});
  EXPECT_THAT(ComputeAndValidateHashes(&cf_), IsOk());
  AssertAllFieldsHaveHashes();
  EXPECT_EQ(cf_.capsules[0].fields[0].hashes.size(), 3);
}

TEST_F(UhhFixture, CollidingAliases) {
  cf_.capsules[0].fields[0].attributes.push_back({"former_name", "donkey"});
  cf_.capsules[0].fields[0].attributes.push_back({"former_name", "ape"});
  cf_.capsules[0].fields[1].attributes.push_back({"former_name", "giraffe"});
  cf_.capsules[0].fields[1].attributes.push_back({"former_name", "ape"});
  EXPECT_THAT(ComputeAndValidateHashes(&cf_).ToString(), HasSubstr("ape"));
}

}  // namespace
