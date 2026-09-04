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

  Code Lookup(core::CRC32C h, uint32_t* out) const {
    const auto i = map_.find(h);
    if (i == map_.end()) {
      return Code::kNotFound;
    }
    *out = i->second;
    return Code::kOk;
  }

  Code Insert(core::CRC32C h, uint32_t value) {
    const auto i = map_.find(h);
    if (i == map_.end()) {
      map_.insert(std::make_pair(h, value));
      return Code::kOk;
    }
    return Code::kPrecondition;
  }

  // Builds a populated map from the offset table in a serialized capsule.
  static ResultOr<ViewMapper> Build(const abi::OffsetTableEntry* ot,
                                    size_t num_entries);

 private:
  std::flat_map<core::CRC32C, uint32_t> map_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_VIEW_MAPPER_H_
