#include <memory>
#include <vector>

#include "capsule/storage_factory.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

namespace {

using capsule::NewHeapStorageFactory;
using capsule::StorageFactory;

TEST(HeapStorageFactoryTest_CreateFactory) {
  ResultOr<std::shared_ptr<StorageFactory>> factory_result =
      NewHeapStorageFactory();
  ASSERT_TRUE(factory_result.IsOk());
  std::shared_ptr<StorageFactory> factory =
      std::move(factory_result).ValueOrDie();
  EXPECT_NE(factory, nullptr);
}

TEST(HeapStorageFactoryTest_AllocateAlloc) {
  std::shared_ptr<StorageFactory> factory =
      NewHeapStorageFactory().ValueOrDie();

  auto alloc = factory->NewAlloc(64).ValueOrDie();
  EXPECT_EQ(alloc->n(), 64);
}

TEST(HeapStorageFactoryTest_ConcurrentAllocation) {
  std::shared_ptr<StorageFactory> factory =
      NewHeapStorageFactory().ValueOrDie();

  constexpr int kNumThreads = 4;
  constexpr int kAllocationsPerThread = 50;

  std::vector<std::unique_ptr<Thread>> threads;
  threads.reserve(kNumThreads);

  for (int i = 0; i < kNumThreads; ++i) {
    threads.push_back(CreateThread("alloc_thread", [factory = factory.get()]() {
      for (int j = 0; j < kAllocationsPerThread; ++j) {
        auto alloc = factory->NewAlloc(32).ValueOrDie();
        CHECK_EQ(alloc->n(), size_t{32});
      }
    }));
  }
}

}  // namespace
