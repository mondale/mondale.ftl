#include <cstdlib>

#include "capsule/storage_factory.h"

namespace capsule {
namespace {

void* ToVoid(const char* c) {
  return reinterpret_cast<void*>(const_cast<char*>(c));
}

char* ToChar(void* v) { return reinterpret_cast<char*>(v); }

class HeapStorageSpan final : public StorageSpan {
 public:
  HeapStorageSpan(size_t bytes) : StorageSpan(NewAsSv(bytes)) {}
  ~HeapStorageSpan() { std::free(ToVoid(span().data())); }

 private:
  static size_t RoundUp8(size_t n) { return (n + 7) / 8; }
  static std::string_view NewAsSv(size_t b) {
    return std::string_view(ToChar(std::aligned_alloc(8, RoundUp8(b))), b);
  }
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
