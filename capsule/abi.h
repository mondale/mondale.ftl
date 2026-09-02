#ifndef CAPSULE_ABI_H_
#define CAPSULE_ABI_H_

#include <stdint.h>

#include "core/crc32c.h"

namespace capsule::abi {

struct FrameHeader final {
  core::CRC32C capsule_id_hash;
  uint32_t frame_length;
};
static_assert(sizeof(FrameHeader) == 8, "FrameHeader needs to be 2 DWORDS.");

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

}  // namespace capsule::abi

#endif  // #ifndef CAPSULE_ABI_H_
