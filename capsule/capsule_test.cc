#include "capsule/capsule.h"
#include "capsule/capsule_test.capsule.h"
#include "capsule/storage_factory.h"
#include "testing/testing.h"

using testing::IsOk;
using testing::Not;

namespace {

class CapsuleTestFixture : public ::testing::Test {
 protected:
  std::shared_ptr<capsule::Storage> Allocate(size_t n) {
    return capsule::Storage::Allocate(factory_, n).ValueOrDie();
  }

  std::shared_ptr<capsule::StorageFactory> factory_ =
      capsule::NewHeapStorageFactory().ValueOrDie();
};

TEST_F(CapsuleTestFixture, BasicSerDes) {
  capsule_test::TestCapsuleM cap;
  cap.code = 100;
  cap.message = "Hello!";
  auto s = Allocate(cap.ComputeStorageSize());
  ASSERT_THAT(capsule::Serialize(cap, s), IsOk());

  capsule_test::TestCapsuleV view;
  ASSERT_THAT(capsule::Deserialize(&view, s), IsOk());
  EXPECT_EQ(view.code, 100);
  EXPECT_EQ(view.message, "Hello!");
}

TEST_F(CapsuleTestFixture, DeserializeUnalignedOffset) {
  auto s = Allocate(32);
  capsule_test::TestCapsuleV view;
  EXPECT_THAT(capsule::Deserialize(&view, s, 4), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, DeserializeOffsetExceedsSize) {
  auto s = Allocate(32);
  capsule_test::TestCapsuleV view;
  EXPECT_THAT(capsule::Deserialize(&view, s, 33), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, DeserializeSizeTooSmall) {
  auto s = Allocate(8);
  capsule_test::TestCapsuleV view;
  EXPECT_THAT(capsule::Deserialize(&view, s, 0), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, SerializeUnalignedOffset) {
  capsule_test::TestCapsuleM cap;
  cap.code = 100;
  cap.message = "Hello!";
  auto s = Allocate(64);
  EXPECT_THAT(capsule::Serialize(cap, s, 4), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, SerializeOffsetExceedsSize) {
  capsule_test::TestCapsuleM cap;
  cap.code = 100;
  cap.message = "Hello!";
  auto s = Allocate(32);
  EXPECT_THAT(capsule::Serialize(cap, s, 33), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, SerializeSizeTooSmall) {
  capsule_test::TestCapsuleM cap;
  cap.code = 100;
  cap.message = "Hello!";
  auto s = Allocate(cap.ComputeStorageSize() - 4);
  EXPECT_THAT(capsule::Serialize(cap, s, 0), Not(IsOk()));
}

TEST_F(CapsuleTestFixture, NonZeroValidOffsetSerDes) {
  capsule_test::TestCapsuleM cap;
  cap.code = 200;
  cap.message = "Offset test";
  const size_t offset = 8;
  auto s = Allocate(cap.ComputeStorageSize() + offset);

  ASSERT_THAT(capsule::Serialize(cap, s, offset), IsOk());

  capsule_test::TestCapsuleV view;
  ASSERT_THAT(capsule::Deserialize(&view, s, offset), IsOk());
  EXPECT_EQ(view.code, 200);
  EXPECT_EQ(view.message, "Offset test");
}

}  // namespace
