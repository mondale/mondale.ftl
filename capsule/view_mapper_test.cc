#include "capsule/abi.h"
#include "capsule/view_mapper.h"
#include "core/crc32c.h"
#include "testing/testing.h"

using capsule::ViewMapper;
using core::CRC32C;
using testing::HasSubstr;
using testing::IsOk;
using testing::Not;

namespace {

TEST(ItsAnInsertOnlyMap) {
  ViewMapper v;
  ASSERT_THAT(v.Insert(CRC32C(0), 0), IsOk());
  ASSERT_THAT(v.Insert(CRC32C(1), 1), IsOk());
  ASSERT_THAT(v.Insert(CRC32C(2), 2), IsOk());
  uint32_t val = 555;
  ASSERT_EQ(Code::kOk, v.Lookup(CRC32C(0), &val));
  EXPECT_EQ(0, val);
  ASSERT_EQ(Code::kOk, v.Lookup(CRC32C(1), &val));
  EXPECT_EQ(1, val);
  ASSERT_EQ(Code::kOk, v.Lookup(CRC32C(2), &val));
  EXPECT_EQ(2, val);

  ASSERT_EQ(Code::kPrecondition, v.Insert(CRC32C(2), 44));
  EXPECT_EQ(Code::kNotFound, v.Lookup(CRC32C(4), &val));
}

TEST(BuildFromOt) {
  capsule::abi::OffsetTableEntry ot[3];
  for (int i = 0; i < 3; ++i) {
    ot[i].field_hash = CRC32C(i);
    ot[i].value = i * 100 + 30;
  }
  auto v = ViewMapper::Build(ot, 3).ValueOrDie();
  uint32_t val = 555;
  EXPECT_EQ(Code::kOk, v.Lookup(CRC32C(0), &val));
  EXPECT_EQ(val, 0 * 100 + 30);
  EXPECT_EQ(Code::kOk, v.Lookup(CRC32C(1), &val));
  EXPECT_EQ(val, 1 * 100 + 30);
  EXPECT_EQ(Code::kOk, v.Lookup(CRC32C(2), &val));
  EXPECT_EQ(val, 2 * 100 + 30);
}

TEST(BuildFromBadOt) {
  capsule::abi::OffsetTableEntry ot[3];
  for (int i = 0; i < 3; ++i) {
    ot[i].field_hash = CRC32C(1);
    ot[i].value = i * 100 + 30;
  }
  EXPECT_THAT(ViewMapper::Build(ot, 3).result(), Not(IsOk()));
}

}  // namespace
