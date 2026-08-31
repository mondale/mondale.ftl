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
  EXPECT_EQ(0, v.Lookup(CRC32C(0)).ValueOrDie());
  EXPECT_EQ(1, v.Lookup(CRC32C(1)).ValueOrDie());
  EXPECT_EQ(2, v.Lookup(CRC32C(2)).ValueOrDie());

  ASSERT_THAT(v.Insert(CRC32C(2), 44), Not(IsOk())) << "No redundant keys.";
  EXPECT_THAT(v.Lookup(CRC32C(4)).result().ToString(), HasSubstr("not found"));
}

TEST(BuildFromOt) {
  capsule::abi::OffsetTableEntry ot[3];
  for (int i = 0; i < 3; ++i) {
    ot[i].field_hash = CRC32C(i);
    ot[i].value = i * 100 + 30;
  }
  auto v = ViewMapper::Build(ot, 3).ValueOrDie();
  EXPECT_EQ(v.Lookup(CRC32C(0)).ValueOrDie(), 0 * 100 + 30);
  EXPECT_EQ(v.Lookup(CRC32C(1)).ValueOrDie(), 1 * 100 + 30);
  EXPECT_EQ(v.Lookup(CRC32C(2)).ValueOrDie(), 2 * 100 + 30);
}

TEST(BuildFromBadOt) {
  capsule::abi::OffsetTableEntry ot[3];
  for (int i = 0; i < 3; ++i) {
    ot[i].field_hash = CRC32C(1);
    ot[i].value = i * 100 + 30;
  }
  EXPECT_THAT(ViewMapper::Build(ot, 3).result().ToString(),
              HasSubstr("already present"));
}

}  // namespace
