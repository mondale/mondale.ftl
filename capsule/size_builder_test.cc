#include "capsule/size_builder.h"
#include "testing/testing.h"

namespace capsule {

TEST(EmptyCapsuleTest) {
  EXPECT_EQ(sizeof(abi::Header), SizeBuilder().Build());
}

TEST(SmallPrimitivesCostOneOte) {
  constexpr size_t kOne = sizeof(abi::Header) + sizeof(abi::OffsetTableEntry);
  EXPECT_EQ(kOne, SizeBuilder().Add8bField().Build());
  EXPECT_EQ(kOne, SizeBuilder().Add16bField().Build());
  EXPECT_EQ(kOne, SizeBuilder().Add32bField().Build());
}

TEST(SixtyFoursCostMore) {
  EXPECT_EQ(sizeof(abi::Header) + sizeof(abi::OffsetTableEntry) + 8,
            SizeBuilder().Add64bField().Build());
}

TEST(StringFragmentation) {
  EXPECT_EQ(sizeof(abi::Header) + sizeof(abi::OffsetTableEntry) + 8,
            SizeBuilder().AddVariableLengthField(3).Build());
}

}  // namespace capsule
