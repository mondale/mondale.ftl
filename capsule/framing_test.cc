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

void PopulateUnsignedOkFramedCapsule(FramedCapsule* c) {
  memset(c, 0, sizeof(*c));
  c->fh.frame_length = sizeof(FramedCapsule);
  c->fh.frame_type =
      static_cast<uint32_t>(capsule::abi::FrameType::kChecksummed);
  c->ih.capsule_length = sizeof(FramedCapsule) -
                         sizeof(capsule::abi::FrameHeader) -
                         sizeof(capsule::abi::ChecksummedFrameFooter);
  c->cff.reiterated_frame_type =
      static_cast<uint32_t>(capsule::abi::FrameType::kChecksummed);
  c->cff.reiterated_frame_length = sizeof(FramedCapsule);
  c->ih.offset_table_size = 3;
  for (int i = 0; i < 3; ++i) {
    c->ot[i].value = offsetof(FramedCapsule, data[i]);
  }
}

std::unique_ptr<FramedCapsule> MakeUnsignedOkFramedCapsule() {
  auto c = std::make_unique<FramedCapsule>();
  PopulateUnsignedOkFramedCapsule(c.get());
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
  c->ih.offset_table_size = 0;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("encodes empty offset table"));
  c->ih.offset_table_size = 9999;
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("encodes offset table count [9999]"));
}

TEST(CrcFail) {
  auto c = MakeUnsignedOkFramedCapsule();
  ASSERT_THAT(Framing::Validate(c.get(), sizeof(FramedCapsule)).ToString(),
              HasSubstr("CRC32C"));
}

class FramingFixture : public ::testing::Test {
 protected:
  std::shared_ptr<capsule::StorageFactory> fac_ =
      capsule::NewHeapStorageFactory().ValueOrDie();
};

TEST_F(FramingFixture, FrameAlloc) {
  const size_t framesize = sizeof(capsule::abi::FrameHeader) +
                           sizeof(capsule::abi::ChecksummedFrameFooter);
  const size_t capsize = sizeof(FramedCapsule) - framesize;
  auto fc = Framing::AllocFrame(capsize, fac_).ValueOrDie();
  EXPECT_EQ(fc.frame_storage->n(), framesize + capsize);
  EXPECT_EQ(fc.capsule_storage->n(), capsize);
  auto* const cap = reinterpret_cast<FramedCapsule*>(fc.frame_storage->base());
  PopulateUnsignedOkFramedCapsule(cap);
  ASSERT_THAT(Framing::CompleteFraming(core::CRC32C(4), &fc), IsOk());

  auto unframed = Framing::Unframe(fc.frame_storage.get()).ValueOrDie();
  EXPECT_EQ(core::CRC32C(4), unframed.enclosed_type);
  EXPECT_EQ(fc.capsule_storage->base(), unframed.storage->base());
}

}  // namespace
