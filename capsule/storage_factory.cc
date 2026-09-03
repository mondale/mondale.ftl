#include <cstdlib>

#include "capsule/storage_factory.h"

namespace capsule {
namespace {

class HeapStorageSpan final : public StorageSpan {
 public:
  HeapStorageSpan(size_t bytes) : StorageSpan(New(bytes), bytes) {}
  ~HeapStorageSpan() { std::free(DataAsPtrTo<void>()); }

 private:
  static size_t RoundUp8(size_t n) { return (n + 7) / 8 * 8; }
  static void* New(size_t b) { return std::aligned_alloc(8, RoundUp8(b)); }
};

class HeapStorageFactory final : public StorageFactory {
 public:
  ~HeapStorageFactory() override {}

  ResultOr<std::unique_ptr<StorageSpan>> NewSpan(size_t size_bytes) final {
    return {std::make_unique<HeapStorageSpan>(size_bytes)};
  }
};

}  // namespace

StorageFactory::~StorageFactory() {}
StorageSpan::~StorageSpan() {}

ResultOr<std::unique_ptr<StorageFactory>> NewHeapStorageFactory() {
  return {std::make_unique<HeapStorageFactory>()};
}

}  // namespace capsule
