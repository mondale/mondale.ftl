#ifndef CAPSULE_STORAGE_FACTORY_H_
#define CAPSULE_STORAGE_FACTORY_H_

#include <memory>

#include "core/vocabulary.h"

namespace capsule {

// StorageFactory subclases must be thread-safe.
class StorageFactory {
 public:
  class Alloc {
   public:
    Alloc(void* d, size_t n) : data_(d), n_(n) {}
    virtual ~Alloc() = 0;

    void* data() const { return data_; }
    size_t n() const { return n_; }

   private:
    void* const data_;
    size_t n_;
  };

  virtual ~StorageFactory() = 0;

  // Allocate and return a new span of `size_bytes` length.
  //
  // Return codes callers must handle or propagate:
  //  * Code::kExhausted - when the underlying allocator is exhausted.
  //  * Additional implementation-specific error conditions.
  virtual ResultOr<std::shared_ptr<Alloc>> NewAlloc(size_t size_bytes) = 0;

 private:
};

// Returns a simple StorageFactory that just uses the C++ heap.
ResultOr<std::shared_ptr<StorageFactory>> NewHeapStorageFactory();

}  // namespace capsule

#endif  // #ifndef CAPSULE_STORAGE_FACTORY_H_
