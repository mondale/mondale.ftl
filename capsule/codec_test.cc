#include <string.h>

#include <cstddef>
#include <memory>

#include "capsule/abi.h"
#include "capsule/codec.h"
#include "testing/testing.h"

using capsule::Codec;
using testing::HasSubstr;
using testing::IsOk;

namespace {

struct Capsule {
  capsule::abi::Header h;
  capsule::abi::OffsetTableEntry ot[3];
  uint64_t data[8];
  core::CRC32C crc;
};

std::unique_ptr<Capsule> MakeUnsignedOkCapsule() {
  auto c = std::make_unique<Capsule>();
  memset(c.get(), 0, sizeof(*c));
  c->h.capsule_length = sizeof(Capsule);
  c->h.capsule_length_reiteration = sizeof(Capsule);
  c->h.offset_table_count = 3;
  for (int i = 0; i < 3; ++i) {
    c->ot[i].value = offsetof(Capsule, data[i]);
  }
  return c;
}

TEST(SignAndValidate) {
  auto c = MakeUnsignedOkCapsule();
  ASSERT_THAT(Codec::Sign(c.get(), sizeof(Capsule)), IsOk());
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)), IsOk());
}

TEST(UnalignedAddress) {
  auto c = MakeUnsignedOkCapsule();
  ASSERT_THAT(
      Codec::Validate(reinterpret_cast<char*>(c.get()) + 1, sizeof(Capsule) - 3)
          .ToString(),
      HasSubstr("Storage address"));
}

TEST(MinLength) {
  auto c = MakeUnsignedOkCapsule();
  ASSERT_THAT(Codec::Validate(c.get(), 4).ToString(),
              HasSubstr("less than minimum"));
}

TEST(LengthCongruency) {
  auto c = MakeUnsignedOkCapsule();
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule) - 1).ToString(),
              HasSubstr("not a multiple of 4"));
}

TEST(LengthAgreement) {
  auto c = MakeUnsignedOkCapsule();
  c->h.capsule_length_reiteration++;
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)).ToString(),
              HasSubstr("differs from reiterated"));
  c->h.capsule_length = c->h.capsule_length_reiteration;
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)).ToString(),
              HasSubstr("differs from framed"));
}

TEST(OteCount) {
  auto c = MakeUnsignedOkCapsule();
  c->h.offset_table_count = 0;
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)).ToString(),
              HasSubstr("encodes empty offset table"));
  c->h.offset_table_count = 9999;
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)).ToString(),
              HasSubstr("encodes offset table count [9999]"));
}

TEST(CrcFail) {
  auto c = MakeUnsignedOkCapsule();
  ASSERT_THAT(Codec::Validate(c.get(), sizeof(Capsule)).ToString(),
              HasSubstr("CRC32C"));
}

}  // namespace
