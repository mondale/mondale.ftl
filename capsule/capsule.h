#ifndef CAPSULE_CAPSULE_H_
#define CAPSULE_CAPSULE_H_

#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "core/vocabulary.h"

namespace capsule {

// API to deserialize a materialized capsule or capsule view from storage.
//
// Note that deserialized from a framed format should use the helpers below.
template <typename T>
Result Deserialize(T* out, const Storage* s);

// API to serialize a materialized capsule to storage.
//
// Note that serialization to a framed format should use the helpers below.
template <typename T>
Result Serialize(const T& in, Storage* s);

///// Implementation follows.

Result DeserializeArgsCheck(const Storage* s);

constexpr size_t kMinimumCapsuleSizeBytes = 16;
template <typename T>
Result Deserialize(T* out, const Storage* s) {
  TRY(DeserializeArgsCheck(s));
  TRY_ASSIGN(auto d, Decoder::Build(s->base(), s->n()));
  return out->Decode(&d);
}

Result SerializeArgsCheck(const Storage* s, size_t capsule_size);
Result SealSizesDiffer(size_t seal_size, size_t capsule_size);

template <typename T>
Result Serialize(const T& in, Storage* s) {
  const auto size = in.ComputeStorageSize();
  TRY(SerializeArgsCheck(s, size));
  auto* const base = s->DataAsPtrTo<char>();
  capsule::Encoder e(base, size, T::kFieldCount);
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
