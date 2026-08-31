#ifndef CAPSULE_ABI_H_
#define CAPSULE_ABI_H_

#include <stdint.h>

#include "core/crc32c.h"

namespace capsule::abi {

struct Header {
  core::CRC32C capsule_id_hash;
  uint32_t capsule_length;
  uint32_t offset_table_count;
  uint32_t capsule_length_reiteration;
};
static_assert(sizeof(Header) == 16, "Header needs to be 4 DWORDS.");

struct OffsetTableEntry {
  core::CRC32C field_hash;
  uint32_t value;  // either a pointer, or the literal value.
};
static_assert(sizeof(OffsetTableEntry) == 8,
              "Offset table entry needs to be 2 DWORDS.");

}  // namespace capsule::abi

#endif  // #ifndef CAPSULE_ABI_H_
