#include "capsule/capsule.h"
#include "capsule/framing.h"

namespace capsule {

Result DeserializeArgsCheck(const Storage* s) {
  auto* const base = s->DataAsPtrTo<char>();
  const uintptr_t addr = reinterpret_cast<uintptr_t>(base);
  if (0 != (addr % 8)) {
    return core::InvalidArgumentError(
        strings::Format("Capsule deserialization must occur at address "
                        "congruent to zero mod 8 [0x{:016x}]",
                        addr));
  }
  const auto size = s->n();
  if (size < kMinimumCapsuleSizeBytes) {
    return core::InvalidArgumentError(strings::Format(
        "A minimum size of [{}] bytes is needed for deserialization. Storage "
        "size [{}].",
        kMinimumCapsuleSizeBytes, s->n()));
  }
  return Result::Ok();
}

Result SerializeArgsCheck(const Storage* s, size_t capsule_size) {
  auto* const base = s->DataAsPtrTo<char>();
  const uintptr_t addr = reinterpret_cast<uintptr_t>(base);
  if (0 != (addr % 8)) {
    return core::InvalidArgumentError(
        strings::Format("Capsule serialization must occur at address "
                        "congruent to zero mod 8 [0x{:016x}]",
                        addr));
  }
  const auto size = s->n();
  if (size < capsule_size) {
    return core::InvalidArgumentError(
        strings::Format("Capsule requires a minimum size of [{}] bytes for "
                        "serialization. Storage "
                        "size [{}].",
                        capsule_size, s->n()));
  }
  return Result::Ok();
}

Result SealSizesDiffer(size_t seal_size, size_t capsule_size) {
  return core::CapsuleFatalError(
      strings::Format("After encoding capsule reports size [{}], not equal to "
                      "predicted size [{}].",
                      seal_size, capsule_size));
}

ResultOr<UnframedCapsule> UnframeAndReportType(const Storage* s) {
  TRY_ASSIGN(auto uc, Framing::Unframe(s));
  UnframedCapsule ret;
  ret.enclosed_type = uc.enclosed_type;
  ret.storage = std::move(uc.storage);
  return ret;
}

}  // namespace capsule
