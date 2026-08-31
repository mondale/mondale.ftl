#include "capsule/view_mapper.h"

namespace capsule {

Result ViewMapper::AlreadyExists(core::CRC32C h) const {
  return Result(
      Code::kPrecondition,
      strings::Format("Capsule hash {:08x} is already present in ViewMapper.",
                      h.value()));
}

Result ViewMapper::NotFound(core::CRC32C h) const {
  return Result(Code::kNotFound,
                strings::Format("Capsule hash {:08x} not found in ViewMapper.",
                                h.value()));
}

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
