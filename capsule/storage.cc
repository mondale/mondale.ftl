#include "capsule/storage.h"

namespace capsule {

Storage::Storage(std::unique_ptr<StorageSpan> s) : span_(std::move(s)) {}

// static
ResultOr<std::shared_ptr<Storage>> Storage::Allocate(StorageFactory* f,
                                                     size_t n) {
  TRY_ASSIGN(auto s, f->NewSpan(n));
  return std::make_shared<Storage>(std::move(s));
}

}  // namespace capsule
