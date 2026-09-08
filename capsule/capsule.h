#ifndef CAPSULE_CAPSULE_H_
#define CAPSULE_CAPSULE_H_

#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/storage.h"
#include "core/vocabulary.h"

namespace capsule {

// API to deserialize a materialized capsule or capsule view from storage.
//
// Derserialization may optionaly begin at an offset relative to the base
// pointer for 's'.
template <typename T>
Result Deserialize(T* out, std::shared_ptr<Storage> s, size_t offset = 0);

// API to serialize a materialized capsule to storage, optionally starting at a
// nonzero offset relative to the base pointer for 's'.
template <typename T>
Result Serialize(const T& in, std::shared_ptr<Storage> s, size_t offset = 0);

///// Implementation follows.

Result DeserializeArgsCheck(const std::shared_ptr<Storage>& s, size_t offset);

constexpr size_t kMinimumCapsuleSizeBytes = 16;
template <typename T>
Result Deserialize(T* out, std::shared_ptr<Storage> s, size_t offset) {
  TRY(DeserializeArgsCheck(s, offset));
  const auto* const base = s->DataAsPtrTo<const char>();
  const auto* const target = base + offset;
  const auto size = s->n() - offset;
  TRY_ASSIGN(auto d, Decoder::Build(target, size));
  return out->Decode(&d);
}

Result SerializeArgsCheck(const std::shared_ptr<Storage>& s, size_t offset,
                          size_t capsule_size);
Result SealSizesDiffer(size_t seal_size, size_t capsule_size);

template <typename T>
Result Serialize(const T& in, std::shared_ptr<Storage> s, size_t offset) {
  const auto size = in.ComputeStorageSize();
  TRY(SerializeArgsCheck(s, offset, size));
  auto* const base = s->DataAsPtrTo<char>();
  auto* const target = base + offset;
  capsule::Encoder e(target, size, T::kFieldCount);
  in.Encode(&e);
  TRY(e.result());
  const auto seal_size = e.Seal();
  if (seal_size != size) {
    return SealSizesDiffer(seal_size, size);
  }
  return Result::Ok();
}

}  // namespace capsule

#endif  // #ifndef CAPSULE_CAPSULE_H_
