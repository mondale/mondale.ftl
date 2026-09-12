#include "capsule/capsule.h"
#include "capsule/capsule_test.capsule.h"
#include "capsule/storage_factory.h"
#include "testing/testing.h"

using testing::IsOk;
using testing::Not;

namespace {

class CapsuleTestFixture : public ::testing::Test {
 protected:
  CapsuleTestFixture() {
    cap_.code = 100;
    cap_.message = "Hello!";
  }
  std::unique_ptr<capsule::Storage> Allocate(size_t n) {
    return capsule::Storage::Allocate(factory_, n).ValueOrDie();
  }

  capsule_test::TestCapsuleM cap_;
  std::shared_ptr<capsule::StorageFactory> factory_ =
      capsule::NewHeapStorageFactory().ValueOrDie();
};

TEST_F(CapsuleTestFixture, FramedSerDes) {
  auto fs = capsule::SerializeAndFrame(cap_, factory_).ValueOrDie();
  auto uc = capsule::UnframeAndReportType(fs.get()).ValueOrDie();
  ASSERT_EQ(uc.enclosed_type, capsule_test::TestCapsuleM::kTypeHash);

  capsule_test::TestCapsuleV view;
  ASSERT_THAT(capsule::Deserialize(&view, uc.storage.get()), IsOk());
  EXPECT_EQ(view.code, 100);
  EXPECT_EQ(view.message, "Hello!");
}

TEST_F(CapsuleTestFixture, FramelessSerDes) {
  cap_.code = 100;
  cap_.message = "Hello!";
  auto s = Allocate(cap_.ComputeStorageSize());
  ASSERT_THAT(capsule::Serialize(cap_, s.get()), IsOk());

  capsule_test::TestCapsuleV view;
  ASSERT_THAT(capsule::Deserialize(&view, s.get()), IsOk());
  EXPECT_EQ(view.code, 100);
  EXPECT_EQ(view.message, "Hello!");

  capsule_test::TestCapsuleM also_cap;
  ASSERT_THAT(capsule::Deserialize(&also_cap, s.get()), IsOk());
  EXPECT_EQ(also_cap.code, 100);
  EXPECT_EQ(also_cap.message, "Hello!");
}

TEST_F(CapsuleTestFixture, DeserializeSizeTooSmall) {
  auto s = Allocate(8);
  capsule_test::TestCapsuleV view;
  EXPECT_THAT(capsule::Deserialize(&view, s.get()), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, SerializeSizeTooSmall) {
  auto s = Allocate(cap_.ComputeStorageSize() - 4);
  EXPECT_THAT(capsule::Serialize(cap_, s.get()), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, ParseFromToString) {
  capsule_test::TestCapsuleM m;
  EXPECT_THAT(capsule::ParseFromText(&m, cap_.ToString()), IsOk());
  EXPECT_EQ(m.code, cap_.code);
  EXPECT_EQ(m.message, cap_.message);
}

}  // namespace
