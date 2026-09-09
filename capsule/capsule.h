#ifndef CAPSULE_CAPSULE_H_
#define CAPSULE_CAPSULE_H_

#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/framing.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "core/crc32c.h"
#include "core/vocabulary.h"

namespace capsule {

// API to serialize and frame a capsule, returing the frame storage.
template <typename T>
ResultOr<std::unique_ptr<Storage>> SerializeAndFrame(
    const T& in, std::shared_ptr<StorageFactory> fac);

// API to unframe a capsule and report its type for subsequent instantiation.
struct UnframedCapsule {
  core::CRC32C enclosed_type;
  std::unique_ptr<Storage> storage;
};
ResultOr<UnframedCapsule> UnframeAndReportType(const Storage* s);

// API to deserialize a materialized capsule or capsule view from storage.
template <typename T>
Result Deserialize(T* out, const Storage* s);

// API to serialize a materialized capsule to storage.
//
// Note that serialization to a framed format should use the helper above.
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

template <typename T>
ResultOr<std::unique_ptr<Storage>> SerializeAndFrame(
    const T& in, std::shared_ptr<StorageFactory> fac) {
  TRY_ASSIGN(auto fc,
             Framing::AllocFrame(in.ComputeStorageSize(), std::move(fac)));
  TRY(Serialize(in, fc.capsule_storage.get()));
  TRY(Framing::CompleteFraming(T::kTypeHash, &fc));
  return std::move(fc.frame_storage);
}

}  // namespace capsule

#endif  // #ifndef CAPSULE_CAPSULE_H_
