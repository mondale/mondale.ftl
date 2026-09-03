#include "capsule/size_builder.h"
#include "testing/testing.h"

namespace capsule {
namespace {

size_t RoundUpToMultipleOf8(size_t s) { return (s + 7) / 8 * 8; }

}  // namespace

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

TEST(PrimitiveVectors) {
  std::vector<uint64_t> v;
  v.resize(9);
  SizeBuilder sb;
  sb.Add(v);
  EXPECT_EQ(
      RoundUpToMultipleOf8(sizeof(abi::Header) + sizeof(abi::OffsetTableEntry) +
                           4 +  // encoding of element count
                           9 * sizeof(uint64_t)),
      sb.Build());
}

TEST(StuffThatCanComputeItself) {
  struct Awesome {
    size_t ComputeStorageSize() const { return 8; }
  } a;
  SizeBuilder sb;
  sb.Add(a);
  EXPECT_EQ(RoundUpToMultipleOf8(sizeof(abi::Header) +
                                 sizeof(abi::OffsetTableEntry) + 8),
            sb.Build());
}

struct Sneaky {
  size_t ComputeStorageSize() const {
    const auto s = next_size;
    next_size += 8;
    sum += s;
    return s;
  }
  static size_t sum;
  static size_t next_size;
};
size_t Sneaky::next_size = 8;
size_t Sneaky::sum = 0;

TEST(VectorsOfStuffThatCanComputeItself) {
  std::vector<Sneaky> v;
  v.resize(6);
  SizeBuilder sb;
  sb.Add(v);
  EXPECT_EQ(
      RoundUpToMultipleOf8(sizeof(abi::Header) + sizeof(abi::OffsetTableEntry) +
                           sizeof(abi::VectorHeader) + Sneaky::sum),
      sb.Build());
}

}  // namespace capsule
