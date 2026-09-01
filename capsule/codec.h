#ifndef CAPSULE_CODEC_H_
#define CAPSULE_CODEC_H_

#include "capsule/abi.h"
#include "core/vocabulary.h"

namespace capsule {

class Codec final {
 public:
  // APIs for a wire capsule with a full FramedHeader.
  static Result Validate(void* base, size_t n);
  static Result Sign(void* base, size_t n);

  // APIs for an inner capsule with the OTECNT/LENGTH header only.
  static uint32_t* InnerHeaderToOteCount(void* base) {
    // TODO - use abi::InnerHeader here.
    return DwordRelative(base, 0);
  }

  static uint32_t* InnerHeaderToLength(void* base) {
    // TODO - use abi::InnerHeader here.
    return DwordRelative(base, 1);
  }

  static uint32_t PayloadAreaSize(uint32_t capsule_length,
                                  uint32_t field_count) {
    return capsule_length - sizeof(abi::InnerHeader) -
           sizeof(abi::OffsetTableEntry) * field_count;
  }

  static abi::OffsetTableEntry* OteEntryNumber(void* base, uint32_t which) {
    return reinterpret_cast<abi::OffsetTableEntry*>(DwordRelative(
        base, sizeof(abi::InnerHeader) / sizeof(uint32_t) +
                  sizeof(abi::OffsetTableEntry) / sizeof(uint32_t) * which));
  }

  static uint32_t* U32AtOffset(void* base, uint32_t offset) {
    return DwordRelative(base, offset / sizeof(uint32_t));
  }

 private:
  static uint32_t* DwordRelative(void* base, uint32_t count) {
    return reinterpret_cast<uint32_t*>(base) + count;
  }
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_CODEC_H_
