#ifndef CAPSULE_FRAMING_H_
#define CAPSULE_FRAMING_H_

#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "core/crc32c.h"
#include "core/vocabulary.h"

namespace capsule {

class Framing final {
 public:
  // Method to run integrity checks on an encoded frame and remove outer
  // framing.
  struct UnframedCapsule final {
    core::CRC32C enclosed_type;
    std::unique_ptr<Storage> storage;
  };
  static ResultOr<UnframedCapsule> Unframe(const Storage* s);

  // Methods to allocate memory to frame a capsule and then complete the
  // framing.
  struct FramedCapsule final {
    std::unique_ptr<Storage> frame_storage;
    std::unique_ptr<Storage> capsule_storage;
  };
  static ResultOr<FramedCapsule> AllocFrame(
      size_t capsule_size, std::shared_ptr<StorageFactory> fac);
  static Result CompleteFraming(core::CRC32C enclosed_type, FramedCapsule* fc);

  // Helpers for the above, public for testing.
  static Result Validate(const void* base, size_t n);
  static Result Sign(void* base, size_t n);
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_FRAMING_H_
