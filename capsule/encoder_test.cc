#include "capsule/encoder.h"
#include "testing/testing.h"

using capsule::Encoder;
using core::CRC32C;
using testing::HasSubstr;
using testing::IsOk;

namespace {

template <int N>
struct Capsule {
  uint32_t otes;
  uint32_t capsule_length;
  capsule::abi::OffsetTableEntry ot[N];
  uint32_t space[N * 64];
};

class EncoderTest : public ::testing::Test {
 protected:
  EncoderTest() {
    memset(&capsule0_, 0, sizeof(capsule0_));
    memset(&capsule1_, 0, sizeof(capsule1_));
  }

  Capsule<0> capsule0_;
  Capsule<1> capsule1_;
};

TEST_F(EncoderTest, EncodeZeroFields) {
  Encoder e(&capsule0_, sizeof(capsule0_), 0);
  EXPECT_THAT(e.result(), IsOk());
  e.AddU32(CRC32C{0}, 0);
  EXPECT_THAT(e.result().ToString(), HasSubstr("overflow"));
  for (int i = 0; i < 100; ++i) {
    e.AddU32(CRC32C{0}, 0);
  }
  EXPECT_THAT(e.result().ToString(), HasSubstr("overflow"));
}

TEST_F(EncoderTest, EncodeOneU32) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  e.AddU32(CRC32C{44}, 55);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, CRC32C{44});
  EXPECT_EQ(capsule1_.ot[0].value, 55);
}

TEST_F(EncoderTest, EncodeOneI32) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  e.AddI32(CRC32C{99}, -55);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, CRC32C{99});
  EXPECT_EQ(std::bit_cast<int32_t>(capsule1_.ot[0].value), -55);
}

TEST_F(EncoderTest, EncodeOneF32) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const float f = 3.14159;
  e.AddF32(CRC32C{314}, f);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, CRC32C{314});
  EXPECT_EQ(std::bit_cast<float>(capsule1_.ot[0].value), f);
}

TEST_F(EncoderTest, EncodeOneU16) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const uint16_t v = 0x5555u;
  const auto crc = CRC32C{__LINE__};
  e.AddU16(crc, v);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(capsule1_.ot[0].value, 0x16165555u);
}

TEST_F(EncoderTest, EncodeOneI16) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const int16_t v = -992;
  const auto crc = CRC32C{__LINE__};
  e.AddI16(crc, v);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(static_cast<int16_t>(capsule1_.ot[0].value & 0x0FFFFu), v);
  EXPECT_EQ(capsule1_.ot[0].value & 0xFFFF0000u, 0x16160000u);
}

TEST_F(EncoderTest, EncodeOneI8) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const int8_t v = -42;
  const auto crc = CRC32C{__LINE__};
  e.AddI8(crc, v);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(static_cast<int8_t>(capsule1_.ot[0].value & 0x0FFu), v);
  EXPECT_EQ(capsule1_.ot[0].value & 0xFFFFFF00u, 0x88888800u);
}

TEST_F(EncoderTest, EncodeOneU8) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const uint8_t v = 42;
  const auto crc = CRC32C{__LINE__};
  e.AddU8(crc, v);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(static_cast<uint8_t>(capsule1_.ot[0].value & 0x0FFu), v);
  EXPECT_EQ(capsule1_.ot[0].value & 0xFFFFFF00u, 0x88888800u);
}

TEST_F(EncoderTest, EncodeOneBoolean) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  e.AddBoolean(crc, true);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8, len) << "Header plus one field.";
  EXPECT_EQ(capsule1_.space[0], 0);
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(static_cast<uint8_t>(capsule1_.ot[0].value & 0x0Fu), 0x01u);
  EXPECT_EQ(capsule1_.ot[0].value & 0xFFFFFF00u, 0xBBBBBB00u);
}

TEST_F(EncoderTest, EncodeOneU64) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  e.AddU64(crc, 0x1234567890ABCDEFull);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8 + 8, len) << "Header plus one field plus 8B for the u64";
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(capsule1_.ot[0].value, offsetof(Capsule<1>, space));
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&capsule1_.space[0]),
            0x1234567890ABCDEFull);
}

TEST_F(EncoderTest, EncodeOneI64) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  const int64_t val = 0xFEDCBA0987654321ll;
  e.AddI64(crc, val);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8 + 8, len) << "Header plus one field plus 8B for the i64";
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(capsule1_.ot[0].value, offsetof(Capsule<1>, space));
  EXPECT_EQ(*reinterpret_cast<int64_t*>(&capsule1_.space[0]), val);
}

TEST_F(EncoderTest, EncodeOneF64) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  const double val = 41153.7;
  e.AddF64(crc, val);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8 + 8, len) << "Header plus one field plus 8B for the F64";
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(capsule1_.ot[0].value, offsetof(Capsule<1>, space));
  EXPECT_EQ(*reinterpret_cast<double*>(&capsule1_.space[0]), val);
}

TEST_F(EncoderTest, EncodeOneString) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  const char* const kHelloThere = "Hello there!";
  e.AddString(crc, kHelloThere, 13);
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8 + (4 + 13 + 7) / 8 * 8, len)
      << "Header, field, 4-length encoding, length so string rounded up "
         "a multiple of 8.";
  EXPECT_EQ(capsule1_.ot[0].value, offsetof(Capsule<1>, space));
  EXPECT_EQ(13, capsule1_.space[0]);
  EXPECT_EQ(std::string(kHelloThere),
            std::string(reinterpret_cast<const char*>(&capsule1_.space[1])));
}

TEST_F(EncoderTest, EncodeOneStringTooBig) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  e.AddString(crc, nullptr, 99999);
  EXPECT_THAT(e.result().ToString(), HasSubstr("overflow"));
  EXPECT_EQ(capsule1_.space[0], 0);
}

TEST_F(EncoderTest, EncodeNestedCapsule) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  auto e2 = e.AddCapsule(crc, 16 /* len */, 1 /* fields */);
  const auto crc_inner = CRC32C{__LINE__};
  e2.AddU32(crc_inner, 1234u);
  EXPECT_EQ(16, e2.Seal());
  EXPECT_THAT(e2.result(), IsOk());
  EXPECT_THAT(e.result(), IsOk());
  const auto len = e.Seal();
  EXPECT_EQ(8 + 1 * 8 + 16, len)
      << "Inner header, field, 4-length encoding, 16B internal capsule";

  // Inspect inner capsule encoding.
  auto* const inner_header =
      reinterpret_cast<capsule::abi::Header*>(&capsule1_.space[0]);
  EXPECT_EQ(1, inner_header->offset_table_count);
  EXPECT_EQ(16, inner_header->capsule_length);
  auto* const inner_ot =
      reinterpret_cast<capsule::abi::OffsetTableEntry*>(&capsule1_.space[2]);
  EXPECT_EQ(crc_inner, inner_ot->field_hash);
  EXPECT_EQ(1234u, inner_ot->value);

  // Inspect outer capsule encoding.
  EXPECT_EQ(capsule1_.ot[0].field_hash, crc);
  EXPECT_EQ(capsule1_.ot[0].value, offsetof(Capsule<1>, space));
}

TEST_F(EncoderTest, EncodeNestedCapsuleTooBigOutside) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  auto e2 = e.AddCapsule(crc, 9999 /* len */, 1 /* fields */);
  EXPECT_THAT(e.result().ToString(), HasSubstr("Payload overflow"));

  // e2 is harmless.
  e2.AddU32(crc, 4);
  e2.AddU32(crc, 4);
  e2.AddU32(crc, 4);
  e2.AddU32(crc, 4);
}

TEST_F(EncoderTest, EncodeNestedCapsuleTooBigInside) {
  Encoder e(&capsule1_, sizeof(capsule1_), 1);
  const auto crc = CRC32C{__LINE__};
  auto e2 = e.AddCapsule(crc, 16 /* len */, 1 /* fields */);
  const auto crc_inner = CRC32C{__LINE__};
  e2.AddU32(crc_inner, 1234u);
  EXPECT_THAT(e2.result(), IsOk());
  EXPECT_THAT(e.result(), IsOk());

  // This causes inner slot overflow.
  e2.AddU32(crc_inner, 1234u);
  EXPECT_THAT(e2.result().ToString(), HasSubstr("Slot overflow"));

  // And the parent is affeceted.
  EXPECT_THAT(e.result().ToString(), HasSubstr("Slot overflow"));
}

}  // namespace
