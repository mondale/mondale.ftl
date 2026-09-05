#include <string.h>

#include "capsule/decoder.h"
#include "testing/testing.h"

using capsule::Decoder;
using core::CRC32C;
using testing::HasSubstr;
using testing::IsOk;
using testing::StrEq;

namespace {

template <int N>
struct Capsule {
  uint32_t otes;
  uint32_t capsule_length;
  capsule::abi::OffsetTableEntry ot[N];
  uint32_t space[N * 64];
};

struct NestedCapsule {
  capsule::abi::Header outer_ih;
  capsule::abi::OffsetTableEntry outer_ot;
  capsule::abi::Header inner_ih;
  capsule::abi::OffsetTableEntry inner_ot;
};

class DecoderTest : public ::testing::Test {
 protected:
  DecoderTest() {
    memset(&capsule0_, 0, sizeof(capsule0_));
    capsule0_.capsule_length = 8;
    memset(&capsule1_, 0, sizeof(capsule1_));
    capsule1_.otes = 1;
    capsule1_.capsule_length = sizeof(capsule1_);
    memset(&nested_, 0, sizeof(nested_));
    nested_.outer_ih.offset_table_count = 1;
    nested_.outer_ih.capsule_length = sizeof(nested_);
    nested_.inner_ih.offset_table_count = 1;
    nested_.inner_ih.capsule_length = sizeof(nested_) / 2;
  }

  Capsule<0> capsule0_;
  Capsule<1> capsule1_;
  NestedCapsule nested_;
};

TEST_F(DecoderTest, TinyMemoryBuilder) {
  EXPECT_THAT(Decoder::Build(nullptr, 7).result().ToString(),
              HasSubstr("[7] bytes"));
}

TEST_F(DecoderTest, OverlongEncodedMemory) {
  capsule0_.capsule_length = 9;
  EXPECT_THAT(Decoder::Build(&capsule0_, 8).result().ToString(),
              HasSubstr("length"));
}

TEST_F(DecoderTest, TooManyOtes) {
  capsule0_.otes = 3;
  capsule0_.capsule_length = 8;
  EXPECT_THAT(Decoder::Build(&capsule0_, 8).result().ToString(),
              HasSubstr("length"));
}

template <typename T>
void PrimitiveNotFoundTest(Decoder* d, T* t, const T& def) {
  const auto crc = core::CRC32C(99);
  std::vector<bool> presence(1, true);
  ASSERT_NE(*t, def);
  EXPECT_EQ(Code::kOk, d->Find(crc, t, def, presence[0]));
  EXPECT_EQ(*t, def);
  EXPECT_FALSE(presence[0]);
}

TEST_F(DecoderTest, BooleanNotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  bool b = false;
  PrimitiveNotFoundTest(&d, &b, true);
}

TEST_F(DecoderTest, U8NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  uint8_t v = 0;
  const uint8_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, I8NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  int8_t v = 0;
  const int8_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, U16NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  uint16_t v = 0;
  const uint16_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, I16NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  int16_t v = 0;
  const int16_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, U32NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  uint32_t v = 0;
  const uint32_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, I32NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  int32_t v = 0;
  const int32_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, F32NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  float v = 0;
  const float def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, U64NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  uint64_t v = 0;
  const uint64_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, I64NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  int64_t v = 0;
  const int64_t def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, F64NotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  double v = 0;
  const double def = __LINE__;
  PrimitiveNotFoundTest(&d, &v, def);
}

TEST_F(DecoderTest, FindBooleanTrueClean) {
  std::vector<bool> presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0xBBBBBBB1u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  bool b = false;
  bool def = false;
  EXPECT_EQ(Code::kOk, d.Find(crc, &b, def, presence[0]));
  EXPECT_TRUE(b);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindBooleanFalseClean) {
  std::vector<bool> presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0xBBBBBBB0u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  bool b = true;
  bool def = true;
  EXPECT_EQ(Code::kOk, d.Find(crc, &b, def, presence[0]));
  EXPECT_FALSE(b);
  EXPECT_TRUE(presence[0]);
}

/*
TEST_F(DecoderTest, FindBooleanDirty) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0xBBBB7BB1u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(Code::kError,
            d.FindBoolean(capsule1_.ot[0].field_hash).result().code());
}

TEST_F(DecoderTest, FindU8Clean) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x88888803u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(0x03u, d.FindU8(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI8Clean) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x888888FFu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(-1, d.FindI8(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI8Dirty) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x882888FFu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(Code::kError, d.FindI8(capsule1_.ot[0].field_hash).result().code());
}

TEST_F(DecoderTest, FindU16Clean) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x16160803u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(0x0803u, d.FindU16(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI16Clean) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x1616FFFEu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(-2, d.FindI16(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI16Dirty) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x161788FFu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(Code::kError,
            d.FindI16(capsule1_.ot[0].field_hash).result().code());
}

TEST_F(DecoderTest, FindU32) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x1616FFFEu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(0x1616FFFEu, d.FindU32(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI32) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value = 0x1616FFFE;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(0x1616FFFE, d.FindI32(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindF32) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  const auto f = 3.000333f;
  capsule1_.ot[0].value = std::bit_cast<uint32_t>(f);
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(f, d.FindF32(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindU64) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value =
      sizeof(capsule::abi::Header) + sizeof(capsule::abi::OffsetTableEntry);
  *reinterpret_cast<uint64_t*>(&capsule1_.space[0]) = 0xda4eda4eda4eda4eull;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(0xda4eda4eda4eda4eull,
            d.FindU64(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindI64) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value =
      sizeof(capsule::abi::Header) + sizeof(capsule::abi::OffsetTableEntry);
  *reinterpret_cast<int64_t*>(&capsule1_.space[0]) = -2715993493052925362;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(-2715993493052925362,
            d.FindI64(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindF64) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value =
      sizeof(capsule::abi::Header) + sizeof(capsule::abi::OffsetTableEntry);
  *reinterpret_cast<double*>(&capsule1_.space[0]) = 312.459;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(312.459, d.FindF64(capsule1_.ot[0].field_hash).ValueOrDie());
}

TEST_F(DecoderTest, FindString) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value =
      sizeof(capsule::abi::Header) + sizeof(capsule::abi::OffsetTableEntry);
  const char* const kMessage = "Nope!";
  *reinterpret_cast<uint32_t*>(&capsule1_.space[0]) = 5;
  memcpy(&capsule1_.space[1], kMessage, 5);
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_THAT(d.FindString(capsule1_.ot[0].field_hash).ValueOrDie(),
              StrEq(kMessage));
}

TEST_F(DecoderTest, FindStringTooBig) {
  capsule1_.ot[0].field_hash = core::CRC32C(__LINE__);
  capsule1_.ot[0].value =
      sizeof(capsule::abi::Header) + sizeof(capsule::abi::OffsetTableEntry);
  const char* const kMessage = "Nope!";
  *reinterpret_cast<uint32_t*>(&capsule1_.space[0]) = 555;
  memcpy(&capsule1_.space[1], kMessage, 5);
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  EXPECT_EQ(Code::kError,
            d.FindString(capsule1_.ot[0].field_hash).result().code());
}

TEST_F(DecoderTest, FindSubcapsule) {
  nested_.outer_ot.field_hash = core::CRC32C('o');
  nested_.inner_ot.field_hash = core::CRC32C('i');

  nested_.outer_ot.value = offsetof(NestedCapsule, inner_ih);
  ASSERT_EQ(16, nested_.outer_ot.value);
  nested_.inner_ot.value = 9u;
  auto d = Decoder::Build(&nested_, sizeof(nested_)).ValueOrDie();
  auto d2 = d.FindCapsule(core::CRC32C('o')).ValueOrDie();
  EXPECT_EQ(9u, d2.FindU32(core::CRC32C('i')).ValueOrDie());
}

TEST_F(DecoderTest, FindSubcapsuleBogus) {
  nested_.outer_ot.field_hash = core::CRC32C('o');
  nested_.inner_ot.field_hash = core::CRC32C('i');

  nested_.outer_ot.value = offsetof(NestedCapsule, inner_ih);
  ASSERT_EQ(16, nested_.outer_ot.value);
  nested_.inner_ot.value = 999u;
  auto d = Decoder::Build(&nested_, sizeof(nested_)).ValueOrDie();
  auto d2 = d.FindCapsule(core::CRC32C('o')).ValueOrDie();
  EXPECT_EQ(Code::kError, d2.FindString(core::CRC32C('i')).result().code());
}
*/

}  // namespace
