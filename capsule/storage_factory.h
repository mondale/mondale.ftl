#ifndef CAPSULE_STORAGE_FACTORY_H_
#define CAPSULE_STORAGE_FACTORY_H_

#include <memory>
#include <string_view>

#include "core/vocabulary.h"

namespace capsule {

// Holder for Storage memory. Exists to an overridden dtor.
class StorageSpan {
 public:
  explicit StorageSpan(std::string_view s) : span_(s) {}
  virtual ~StorageSpan() = 0;

  std::string_view span() const { return span_; }

 private:
  std::string_view span_;
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
