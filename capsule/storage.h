#ifndef CAPSULE_STORAGE_H_
#define CAPSULE_STORAGE_H_

#include "capsule/storage_factory.h"
#include "core/vocabulary.h"

namespace capsule {

// Handle to an underlying allocation, possibly narrowed. May create additional
// handles, also possibly further narrowed.
class Storage final {
 public:
  // Storage thusly constucted adopts the bounds of 'a'.
  explicit Storage(std::shared_ptr<StorageFactory::Alloc> a);

  // Storage thusly constucted initializes members directly.
  Storage(std::shared_ptr<StorageFactory::Alloc> a, void* d, size_t n);

  // Build a new Storage pointing to bytes inside the current span, formed by
  // removing 'trim_head' bytes from the front and 'trim_tail' bytes from the
  // end.
  ResultOr<std::unique_ptr<Storage>> Carve(size_t trim_head,
                                           size_t trim_tail) const;

  // Take a ref on the underlying Alloc.
  std::unique_ptr<Storage> Ref() const;

  // Prefer this method when constucting a View.
  static ResultOr<std::unique_ptr<Storage>> Allocate(
      std::shared_ptr<StorageFactory> f, size_t n);

  template <typename T>
  T* DataAsPtrTo() const {
    return reinterpret_cast<T*>(base_);
  }

  void* base() const { return base_; }
  size_t n() const { return n_; }

 private:
  void* const base_;
  size_t n_;
  std::shared_ptr<StorageFactory::Alloc> alloc_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_STORAGE_H_
