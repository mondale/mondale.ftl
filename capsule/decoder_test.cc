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

TEST_F(DecoderTest, FindBooleanDirty) {
  std::vector<bool> presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0xBBBB7BB1u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  bool b = true;
  bool def = true;
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &b, def, presence[0]));
}

TEST_F(DecoderTest, FindU8Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x88888842u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint8_t v = 0;
  uint8_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 66);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindU8Dirty) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x88878842u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint8_t v = 0;
  uint8_t def = 0;
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &v, def, presence[0]));
}

TEST_F(DecoderTest, FindI8Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x8888887Fu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int8_t v = 0;
  int8_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 127);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindI8Dirty) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x8800887Fu;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int8_t v = 0;
  int8_t def = 0;
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &v, def, presence[0]));
}

TEST_F(DecoderTest, FindU16Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x16161234u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint16_t v = 0;
  uint16_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 0x1234);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindU16Dirty) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x16171234u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint16_t v = 0;
  uint16_t def = 0;
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &v, def, presence[0]));
}

TEST_F(DecoderTest, FindI16Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x16165678u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int16_t v = 0;
  int16_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 0x5678);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindI16Dirty) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 0x11165678u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int16_t v = 0;
  int16_t def = 0;
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &v, def, presence[0]));
}

TEST_F(DecoderTest, FindU32Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 123456789u;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint32_t v = 0;
  uint32_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 123456789u);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindI32Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = static_cast<uint32_t>(-42);
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int32_t v = 0;
  int32_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, -42);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindF32Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  float expected = 3.14159f;
  uint32_t raw_val;
  memcpy(&raw_val, &expected, sizeof(float));
  capsule1_.ot[0].value = raw_val;
  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  float v = 0.0f;
  float def = 0.0f;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_NEAR_ABS(v, 3.14159f, 1e-5f);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindU64Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 16;
  auto* space_u64 =
      reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(&capsule1_) + 16);
  *space_u64 = 0x123456789ABCDEF0ULL;

  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  uint64_t v = 0;
  uint64_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, 0x123456789ABCDEF0ULL);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindI64Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 16;
  auto* space_i64 =
      reinterpret_cast<int64_t*>(reinterpret_cast<char*>(&capsule1_) + 16);
  *space_i64 = -987654321LL;

  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  int64_t v = 0;
  int64_t def = 0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_EQ(v, -987654321LL);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindF64Clean) {
  std::vector presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 16;
  auto* space_f64 =
      reinterpret_cast<double*>(reinterpret_cast<char*>(&capsule1_) + 16);
  *space_f64 = 2.718281828459045;

  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  double v = 0.0;
  double def = 0.0;
  EXPECT_EQ(Code::kOk, d.Find(crc, &v, def, presence[0]));
  EXPECT_NEAR_ABS(v, 2.718281828459045, 1e-9);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindStringNotFoundDefault) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  std::string s = "initial";
  std::vector<bool> presence(1, true);
  const auto crc = core::CRC32C(__LINE__);
  EXPECT_EQ(Code::kOk, d.Find(crc, &s, "default_val", presence[0]));
  EXPECT_EQ(s, "default_val");
  EXPECT_FALSE(presence[0]);
}

TEST_F(DecoderTest, FindStringFoundClean) {
  std::vector<bool> presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 16;

  char* space_ptr = reinterpret_cast<char*>(&capsule1_) + 16;
  uint32_t str_len = 5;
  memcpy(space_ptr, &str_len, sizeof(str_len));
  memcpy(space_ptr + 4, "Hello", 5);

  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  std::string s = "";
  EXPECT_EQ(Code::kOk, d.Find(crc, &s, "def", presence[0]));
  EXPECT_EQ(s, "Hello");
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindStringOverlongError) {
  std::vector<bool> presence(1, false);
  const auto crc = core::CRC32C(__LINE__);
  capsule1_.ot[0].field_hash = crc;
  capsule1_.ot[0].value = 16;

  char* space_ptr = reinterpret_cast<char*>(&capsule1_) + 16;
  uint32_t str_len = 0xFFFFFFF0u;
  memcpy(space_ptr, &str_len, sizeof(str_len));

  auto d = Decoder::Build(&capsule1_, sizeof(capsule1_)).ValueOrDie();
  std::string s = "";
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &s, "def", presence[0]));
}

/*
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
