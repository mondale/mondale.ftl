#ifndef CAPSULE_FRAMING_H_
#define CAPSULE_FRAMING_H_

#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "core/crc32c.h"
#include "core/vocabulary.h"

namespace capsule {

class Framing final {
 public:
  struct Capsule {
    core::CRC32C enclosed_type;
    std::shared_ptr<Storage> storage;
  };

  static ResultOr<Capsule> Unframe(std::shared_ptr<Storage> storage,
                                   std::shared_ptr<StorageFactory> fac);

  static Result Validate(void* base, size_t n);
  static Result Sign(void* base, size_t n);
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_FRAMING_H_
