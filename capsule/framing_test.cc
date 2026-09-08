#include <string.h>

#include <cstddef>
#include <memory>

#include "capsule/abi.h"
#include "capsule/framing.h"
#include "testing/testing.h"

using capsule::Framing;
using testing::HasSubstr;
using testing::IsOk;

namespace {

struct FramedCapsule {
  capsule::abi::FrameHeader fh;
  capsule::abi::Header ih;
  capsule::abi::OffsetTableEntry ot[3];
  uint64_t data[8];
  capsule::abi::ChecksummedFrameFooter cff;
};

std::unique_ptr<FramedCapsule> MakeUnsignedOkFramedCapsule() {
  auto c = std::make_unique<FramedCapsule>();
  memset(c.get(), 0, sizeof(*c));
  c->fh.frame_length = sizeof(FramedCapsule);
  c->fh.frame_type =
      static_cast<uint32_t>(capsule::abi::FrameType::kChecksummed);
  c->ih.capsule_length = sizeof(FramedCapsule) -
                         sizeof(capsule::abi::FrameHeader) -
                         sizeof(capsule::abi::ChecksummedFrameFooter);
  c->cff.reiterated_frame_type =
      static_cast<uint32_t>(capsule::abi::FrameType::kChecksummed);
  c->cff.reiterated_frame_length = sizeof(FramedCapsule);
  c->ih.offset_table_count = 3;
  for (int i = 0; i < 3; ++i) {
    c->ot[i].value = offsetof(FramedCapsule, data[i]);
  }
  return c;
}

TEST(SignAndValidate) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Sign(c.get(), sizeof(FramedCapsule)), IsOk());
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)), IsOk());
}

TEST(UnalignedAddress) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Validate(reinterpret_cast<char*>(c.get()) + 1,
                                sizeof(FramedCapsule) - 3)
                  .ToString(),
              HasSubstr("Storage address"));
}

TEST(MinLength) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Validate(c.get(), 4).ToString(),
              HasSubstr("less than minimum"));
}

TEST(LengthCongruency) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule) - 1).ToString(),
              HasSubstr("not a multiple of 8"));
}

TEST(TypeChecks) {
  auto c = MakeUnsignedOkFramedCapsule();
  c->fh.frame_type = 0;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("frame type"));
  c->fh.frame_type = c->cff.reiterated_frame_type;
  c->cff.reiterated_frame_type = 0;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("frame type"));
}

TEST(LengthAgreement) {
  auto c = MakeUnsignedOkFramedCapsule();
  c->ih.capsule_length++;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("inconsistent with inner capsule length"));
  c->ih.capsule_length--;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule) + 8).ToString(),
              HasSubstr("differs from memory"));
  c->cff.reiterated_frame_length--;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("footer-encoded"));
}

TEST(OteCount) {
  auto c = MakeUnsignedOkFramedCapsule();
  c->ih.offset_table_count = 0;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("encodes empty offset table"));
  c->ih.offset_table_count = 9999;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("encodes offset table count [9999]"));
}

TEST(CrcFail) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("CRC32C"));
}

}  // namespace
