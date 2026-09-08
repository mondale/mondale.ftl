#include <cstdlib>

#include "capsule/storage_factory.h"

namespace capsule {
namespace {

class HeapAlloc final : public StorageFactory::Alloc {
 public:
  HeapAlloc(size_t bytes) : StorageFactory::Alloc(New(bytes), bytes) {}
  ~HeapAlloc() { std::free(data()); }

 private:
  static size_t RoundUp8(size_t n) { return (n + 7) / 8 * 8; }
  static void* New(size_t b) { return std::aligned_alloc(8, RoundUp8(b)); }
};

class HeapStorageFactory final : public StorageFactory {
 public:
  ~HeapStorageFactory() override {}

  ResultOr<std::shared_ptr<StorageFactory::Alloc>> NewAlloc(
      size_t size_bytes) final {
    return {std::make_shared<HeapAlloc>(size_bytes)};
  }
};

}  // namespace

StorageFactory::~StorageFactory() {}
StorageFactory::Alloc::~Alloc() {}

ResultOr<std::shared_ptr<StorageFactory>> NewHeapStorageFactory() {
  return {std::make_shared<HeapStorageFactory>()};
}

}  // namespace capsule
