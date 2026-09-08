#include "capsule/capsule.h"

namespace capsule {

Result DeserializeArgsCheck(const std::shared_ptr<Storage>& s, size_t offset) {
  auto* const base = s->DataAsPtrTo<char>();
  auto* const target = base + offset;
  const uintptr_t addr = reinterpret_cast<uintptr_t>(target);
  if (0 != (addr % 8)) {
    return core::InvalidArgumentError(
        strings::Format("Capsule deserialization must occur at address "
                        "congruent to zero mod 8 [0x{:016x}]",
                        addr));
  }
  if (offset > s->n()) {
    return core::InvalidArgumentError(
        strings::Format("Offset [{}] exceeds length of provided Storage [{}].",
                        offset, s->n()));
  }
  const auto size = s->n() - offset;
  if (size < kMinimumCapsuleSizeBytes) {
    return core::InvalidArgumentError(strings::Format(
        "A minimum size of [{}] bytes is needed for deserialization. Storage "
        "size [{}], offset [{}].",
        kMinimumCapsuleSizeBytes, s->n(), offset));
  }
  return Result::Ok();
}

Result SerializeArgsCheck(const std::shared_ptr<Storage>& s, size_t offset,
                          size_t capsule_size) {
  auto* const base = s->DataAsPtrTo<char>();
  auto* const target = base + offset;
  const uintptr_t addr = reinterpret_cast<uintptr_t>(target);
  if (0 != (addr % 8)) {
    return core::InvalidArgumentError(
        strings::Format("Capsule serialization must occur at address "
                        "congruent to zero mod 8 [0x{:016x}]",
                        addr));
  }
  if (offset > s->n()) {
    return core::InvalidArgumentError(
        strings::Format("Offset [{}] exceeds length of provided Storage [{}].",
                        offset, s->n()));
  }
  const auto size = s->n() - offset;
  if (size < capsule_size) {
    return core::InvalidArgumentError(
        strings::Format("Capsule requires a minimum size of [{}] bytes for "
                        "serialization. Storage "
                        "size [{}], offset [{}].",
                        capsule_size, s->n(), offset));
  }
  return Result::Ok();
}

Result SealSizesDiffer(size_t seal_size, size_t capsule_size) {
  return core::CapsuleFatalError(
      strings::Format("After encoding capsule reports size [{}], not equal to "
                      "predicted size [{}].",
                      seal_size, capsule_size));
}

}  // namespace capsule
