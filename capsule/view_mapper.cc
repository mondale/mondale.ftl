#include "capsule/view_mapper.h"

namespace capsule {

// static
ResultOr<ViewMapper> ViewMapper::Build(const abi::OffsetTableEntry* ot,
                                       size_t num_entries) {
  ViewMapper v;
  for (int i = 0; i < num_entries; ++i) {
    const abi::OffsetTableEntry& ote = ot[i];
    TRY(v.Insert(ote.field_hash, ote.value));
  }
  return v;
}

}  // namespace capsule
