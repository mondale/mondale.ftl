#ifndef CAPSULE_VIEW_MAPPER_H_
#define CAPSULE_VIEW_MAPPER_H_

#include <flat_map>

#include "capsule/abi.h"
#include "core/crc32c.h"
#include "core/vocabulary.h"

namespace capsule {

// Simple insert-only map to help build Views from stored, serialized capsules.
class ViewMapper final {
 public:
  ViewMapper() = default;

  ResultOr<uint32_t> Lookup(core::CRC32C h) const {
    const auto i = map_.find(h);
    if (i != map_.end()) return i->second;
    return NotFound(h);
  }

  Result Insert(core::CRC32C h, uint32_t value) {
    const auto i = map_.find(h);
    if (i == map_.end()) {
      map_.insert(std::make_pair(h, value));
      return Result::Ok();
    }
    return AlreadyExists(h);
  }

  // Builds a populated map from the offset table in a serialized capsule.
  static ResultOr<ViewMapper> Build(const abi::OffsetTableEntry* ot,
                                    size_t num_entries);

 private:
  Result NotFound(core::CRC32C h) const;
  Result AlreadyExists(core::CRC32C h) const;

  std::flat_map<core::CRC32C, uint32_t> map_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_VIEW_MAPPER_H_
