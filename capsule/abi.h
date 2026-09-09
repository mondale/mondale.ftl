#ifndef CAPSULE_ABI_H_
#define CAPSULE_ABI_H_

#include <stdint.h>

#include "core/crc32c.h"

namespace capsule::abi {

// This only goes [0-7].
enum class FrameType {
  kReserved = 0,
  kChecksummed = 1,
  kFurtureUse0 = 2,  // future encrypted?
  kFurtureUse1 = 3,
  kFurtureUse2 = 4,
  kFurtureUse3 = 5,
  kFurtureUse4 = 6,
  kFurtureUse5 = 7,
};

// TODO - rename fields.
struct FrameHeader final {
  core::CRC32C capsule_id_hash;
  uint32_t frame_length : 29;
  uint32_t frame_type : 3;
};
static_assert(sizeof(FrameHeader) == 8, "FrameHeader needs to be 2 DWORDS.");

struct ChecksummedFrameFooter final {
  uint32_t reiterated_frame_length : 29;
  uint32_t reiterated_frame_type : 3;
  core::CRC32C frame_crc;
};
static_assert(sizeof(ChecksummedFrameFooter) == 8,
              "ChecksummedFrameFooter needs to be 2 DWORDS.");

struct Header final {
  uint32_t offset_table_count;
  uint32_t capsule_length;
};
static_assert(sizeof(Header) == 8, "Header needs to be 2 DWORDS.");

struct OffsetTableEntry final {
  core::CRC32C field_hash;
  uint32_t value;  // either a pointer, or the literal value.
};
static_assert(sizeof(OffsetTableEntry) == 8,
              "Offset table entry needs to be 2 DWORDS.");

struct VectorHeader final {
  uint32_t element_count;
  uint32_t padding;
};
static_assert(sizeof(VectorHeader) == 8, "VectorHeader needs to be 2 DWORDS.");

}  // namespace capsule::abi

#endif  // #ifndef CAPSULE_ABI_H_
