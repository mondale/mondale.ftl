#ifndef CAPSULE_STORAGE_FACTORY_H_
#define CAPSULE_STORAGE_FACTORY_H_

#include <memory>

#include "core/vocabulary.h"

namespace capsule {

// Holder for Storage memory. Exists to provide a dtor override.
class StorageSpan {
 public:
  StorageSpan(void* d, size_t n) : data_(d), n_(n) {}
  virtual ~StorageSpan() = 0;

  template <typename T>
  T* DataAsPtrTo() const {
    return reinterpret_cast<T*>(data_);
  }

  size_t n() const { return n_; }

 private:
  void* const data_;
  size_t n_;
};

// StorageFactory subclases must be thread-safe.
class StorageFactory {
 public:
  virtual ~StorageFactory() = 0;

  // Allocate and return a new span of `size_bytes` length.
  //
  // Return codes callers must handle or propagate:
  //  * Code::kExhausted - when the underlying allocator is exhausted.
  //  * Additional implementation-specific error conditions.
  virtual ResultOr<std::unique_ptr<StorageSpan>> NewSpan(size_t size_bytes) = 0;

 private:
};

// Returns a simple StorageFactory that just uses the C++ heap.
ResultOr<std::unique_ptr<StorageFactory>> NewHeapStorageFactory();

}  // namespace capsule

#endif  // #ifndef CAPSULE_STORAGE_FACTORY_H_
