#ifndef CAPSULE_STORAGE_H_
#define CAPSULE_STORAGE_H_

#include "capsule/storage_factory.h"
#include "core/vocabulary.h"

namespace capsule {

// Storage allows Views to have std::shared_ptr<Storage> while internally
// maintaining a std::unique_ptr<StorageSpan>.
class Storage final {
 public:
  // Prefer this method when constucting a View.
  static ResultOr<std::shared_ptr<Storage>> Allocate(StorageFactory* f,
                                                     size_t n);

  template <typename T>
  T* DataAsPtrTo() const {
    return span_->DataAsPtrTo<T>();
  }

  size_t n() const { return span_->n(); }

  // Public only for make_shared. Do not use directly.
  explicit Storage(std::unique_ptr<StorageSpan> s);

 private:
  std::unique_ptr<StorageSpan> span_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_STORAGE_H_
