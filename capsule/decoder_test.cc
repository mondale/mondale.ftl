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
  const std::string default_val = "default_val";
  EXPECT_EQ(Code::kOk, d.Find(crc, &s, default_val, presence[0]));
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
  const std::string default_val = "def";
  EXPECT_EQ(Code::kOk, d.Find(crc, &s, default_val, presence[0]));
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
  const std::string default_val = "def";
  EXPECT_EQ(Code::kCapsuleFatal, d.Find(crc, &s, default_val, presence[0]));
}

class TinyCapsule final {
 public:
  Result Decode(Decoder* d) {
    std::vector<bool> presence(1, false);
    const uint32_t def = 7;
    EXPECT_EQ(Code::kOk, d->Find(core::CRC32C('i'), &val_, def, presence[0]));
    EXPECT_EQ(val_, 9);
    EXPECT_TRUE(presence[0]);
    return Result::Ok();
  }
  uint32_t val_ = 0;
};

TEST_F(DecoderTest, FindSubcapsule) {
  nested_.outer_ot.field_hash = core::CRC32C('o');
  nested_.inner_ot.field_hash = core::CRC32C('i');

  nested_.outer_ot.value = offsetof(NestedCapsule, inner_ih);
  ASSERT_EQ(16, nested_.outer_ot.value);
  nested_.inner_ot.value = 9u;
  auto d = Decoder::Build(&nested_, sizeof(nested_)).ValueOrDie();
  std::vector<bool> presence(1, false);
  TinyCapsule tc;
  EXPECT_EQ(Code::kOk, d.FindCapsule(core::CRC32C('o'), &tc, tc, presence[0]));
  EXPECT_EQ(tc.val_, 9);
}

TEST_F(DecoderTest, FindSubcapsuleBogus) {
  nested_.outer_ot.field_hash = core::CRC32C('o');
  nested_.inner_ot.field_hash = core::CRC32C('i');

  nested_.outer_ot.value = offsetof(NestedCapsule, inner_ih);
  ASSERT_EQ(16, nested_.outer_ot.value);
  nested_.inner_ih.capsule_length = 999;
  nested_.inner_ot.value = 9u;
  auto d = Decoder::Build(&nested_, sizeof(nested_)).ValueOrDie();
  std::vector<bool> presence(1, false);
  TinyCapsule tc;
  EXPECT_NE(Code::kOk, d.FindCapsule(core::CRC32C('o'), &tc, tc, presence[0]));
}

class VectorTinyCapsule final {
 public:
  Result Decode(Decoder* d) {
    std::vector<bool> presence(1, false);
    const uint32_t def = 5;
    EXPECT_EQ(Code::kOk, d->Find(core::CRC32C(1), &val_, def, presence[0]));
    EXPECT_EQ(val_, 42);
    EXPECT_TRUE(presence[0]);
    return Result::Ok();
  }
  uint32_t val_ = 0;
};

struct VectorCapsuleLayout {
  capsule::abi::Header header;
  capsule::abi::OffsetTableEntry ot;
  uint64_t space[16];
};

TEST_F(DecoderTest, FindCapsuleVectorClean) {
  VectorCapsuleLayout layout;
  memset(&layout, 0, sizeof(layout));
  layout.header.offset_table_count = 1;
  layout.header.capsule_length = sizeof(layout);

  const auto crc = core::CRC32C(100);
  layout.ot.field_hash = crc;
  layout.ot.value = 16;

  char* ptr = reinterpret_cast<char*>(&layout) + 16;

  auto* vh = reinterpret_cast<capsule::abi::VectorHeader*>(ptr);
  vh->element_count = 1;
  vh->padding = 0xda4eda4eu;
  ptr += sizeof(capsule::abi::VectorHeader);

  auto* sub_h = reinterpret_cast<capsule::abi::Header*>(ptr);
  sub_h->offset_table_count = 1;
  sub_h->capsule_length = 24;

  auto* sub_ot = reinterpret_cast<capsule::abi::OffsetTableEntry*>(
      ptr + sizeof(capsule::abi::Header));
  sub_ot->field_hash = core::CRC32C(1);
  sub_ot->value = 42;

  auto d = Decoder::Build(&layout, sizeof(layout)).ValueOrDie();
  std::vector<bool> presence(1, false);
  std::vector<VectorTinyCapsule> vtc_vec;
  EXPECT_EQ(Code::kOk, d.FindCapsuleVector(crc, &vtc_vec, presence[0]));
  EXPECT_EQ(vtc_vec.size(), 1);
  EXPECT_EQ(vtc_vec[0].val_, 42);
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindCapsuleVectorNotFound) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  std::vector<bool> presence(1, true);
  std::vector<VectorTinyCapsule> vtc_vec;
  vtc_vec.push_back(VectorTinyCapsule{});
  EXPECT_EQ(Code::kOk,
            d.FindCapsuleVector(core::CRC32C(100), &vtc_vec, presence[0]));
  EXPECT_TRUE(vtc_vec.empty());
  EXPECT_FALSE(presence[0]);
}

TEST_F(DecoderTest, FindCapsuleVectorBogusPadding) {
  VectorCapsuleLayout layout;
  memset(&layout, 0, sizeof(layout));
  layout.header.offset_table_count = 1;
  layout.header.capsule_length = sizeof(layout);

  const auto crc = core::CRC32C(100);
  layout.ot.field_hash = crc;
  layout.ot.value = 16;

  char* ptr = reinterpret_cast<char*>(&layout) + 16;
  auto* vh = reinterpret_cast<capsule::abi::VectorHeader*>(ptr);
  vh->element_count = 1;
  vh->padding = 0xBADF00Du;

  auto d = Decoder::Build(&layout, sizeof(layout)).ValueOrDie();
  std::vector<bool> presence(1, false);
  std::vector<VectorTinyCapsule> vtc_vec;
  EXPECT_EQ(Code::kCapsuleFatal,
            d.FindCapsuleVector(crc, &vtc_vec, presence[0]));
}

TEST_F(DecoderTest, FindStringVectorClean) {
  struct StringVectorCapsuleLayout {
    capsule::abi::Header header;
    capsule::abi::OffsetTableEntry ot;
    uint64_t space[32];
  };

  StringVectorCapsuleLayout layout;
  memset(&layout, 0, sizeof(layout));
  layout.header.offset_table_count = 1;
  layout.header.capsule_length = sizeof(layout);

  const auto crc = core::CRC32C(200);
  layout.ot.field_hash = crc;
  layout.ot.value = 16;

  char* ptr = reinterpret_cast<char*>(&layout) + 16;
  auto* vh = reinterpret_cast<capsule::abi::VectorHeader*>(ptr);
  vh->element_count = 1;
  vh->padding = 0xda4eda4eu;
  ptr += sizeof(capsule::abi::VectorHeader);

  uint32_t str_len = 5;
  memcpy(ptr, &str_len, sizeof(str_len));
  memcpy(ptr + 4, "Hello", 5);

  auto d = Decoder::Build(&layout, sizeof(layout)).ValueOrDie();
  std::vector<bool> presence(1, false);
  std::vector<std::string> out_vec;
  EXPECT_EQ(Code::kOk, d.FindStringVector(crc, &out_vec, presence[0]));
  EXPECT_EQ(out_vec.size(), 1);
  EXPECT_EQ(out_vec[0], "Hello");
  EXPECT_TRUE(presence[0]);
}

TEST_F(DecoderTest, FindStringVectorNotFound) {
  auto d = Decoder::Build(&capsule0_, sizeof(capsule0_)).ValueOrDie();
  std::vector<bool> presence(1, true);
  std::vector<std::string> out_vec = {"initial"};
  EXPECT_EQ(Code::kOk,
            d.FindStringVector(core::CRC32C(200), &out_vec, presence[0]));
  EXPECT_TRUE(out_vec.empty());
  EXPECT_FALSE(presence[0]);
}

TEST_F(DecoderTest, FindStringVectorBogusPadding) {
  struct StringVectorCapsuleLayout {
    capsule::abi::Header header;
    capsule::abi::OffsetTableEntry ot;
    uint64_t space[16];
  };

  StringVectorCapsuleLayout layout;
  memset(&layout, 0, sizeof(layout));
  layout.header.offset_table_count = 1;
  layout.header.capsule_length = sizeof(layout);

  const auto crc = core::CRC32C(200);
  layout.ot.field_hash = crc;
  layout.ot.value = 16;

  char* ptr = reinterpret_cast<char*>(&layout) + 16;
  auto* vh = reinterpret_cast<capsule::abi::VectorHeader*>(ptr);
  vh->element_count = 1;
  vh->padding = 0xBADF00Du;

  auto d = Decoder::Build(&layout, sizeof(layout)).ValueOrDie();
  std::vector<bool> presence(1, false);
  std::vector<std::string> out_vec;
  EXPECT_EQ(Code::kCapsuleFatal,
            d.FindStringVector(crc, &out_vec, presence[0]));
}

}  // namespace
