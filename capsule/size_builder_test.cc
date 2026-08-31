#include "capsule/size_builder.h"
#include "testing/testing.h"

namespace capsule {

constexpr size_t kMin = sizeof(abi::Header) + sizeof(core::CRC32C);

TEST(EmptyCapsuleTest) { EXPECT_EQ(kMin, SizeBuilder().Build()); }

TEST(SmallPrimitivesCostOneOte) {
  constexpr size_t kOne = kMin + sizeof(abi::OffsetTableEntry);
  EXPECT_EQ(kOne, SizeBuilder().Add8bField().Build());
  EXPECT_EQ(kOne, SizeBuilder().Add16bField().Build());
  EXPECT_EQ(kOne, SizeBuilder().Add32bField().Build());
}

TEST(SixtyFoursCostMore) {
  EXPECT_EQ(kMin + sizeof(abi::OffsetTableEntry) + 8,
            SizeBuilder().Add64bField().Build());
}

TEST(StringFragmentation) {
  EXPECT_EQ(kMin + sizeof(abi::OffsetTableEntry) + 8,
            SizeBuilder().AddVariablelengthField(3).Build());
}

}  // namespace capsule
