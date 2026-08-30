#include <memory>
#include <vector>

#include "capsule/storage_factory.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

namespace {

using capsule::NewHeapStorageFactory;
using capsule::StorageFactory;
using capsule::StorageSpan;

TEST(HeapStorageFactoryTest_CreateFactory) {
  ResultOr<std::unique_ptr<StorageFactory>> factory_result =
      NewHeapStorageFactory();
  ASSERT_TRUE(factory_result.IsOk());
  std::unique_ptr<StorageFactory> factory =
      std::move(factory_result).ValueOrDie();
  EXPECT_NE(factory, nullptr);
}

TEST(HeapStorageFactoryTest_AllocateSpan) {
  std::unique_ptr<StorageFactory> factory =
      NewHeapStorageFactory().ValueOrDie();

  ResultOr<std::unique_ptr<StorageSpan>> span_result = factory->NewSpan(64);
  ASSERT_TRUE(span_result.IsOk());

  std::unique_ptr<StorageSpan> span = std::move(span_result).ValueOrDie();
  ASSERT_NE(span, nullptr);

  EXPECT_EQ(span->n(), 64);
}

TEST(HeapStorageFactoryTest_AllocateZeroBytes) {
  std::unique_ptr<StorageFactory> factory =
      NewHeapStorageFactory().ValueOrDie();

  ResultOr<std::unique_ptr<StorageSpan>> span_result = factory->NewSpan(0);
  ASSERT_TRUE(span_result.IsOk());

  std::unique_ptr<StorageSpan> span = std::move(span_result).ValueOrDie();
  ASSERT_NE(span, nullptr);
  EXPECT_EQ(span->n(), 0);
}

TEST(HeapStorageFactoryTest_ConcurrentAllocation) {
  std::unique_ptr<StorageFactory> factory =
      NewHeapStorageFactory().ValueOrDie();

  constexpr int kNumThreads = 4;
  constexpr int kAllocationsPerThread = 50;

  std::vector<std::unique_ptr<Thread>> threads;
  threads.reserve(kNumThreads);

  for (int i = 0; i < kNumThreads; ++i) {
    threads.push_back(CreateThread("alloc_thread", [factory = factory.get()]() {
      for (int j = 0; j < kAllocationsPerThread; ++j) {
        auto span = factory->NewSpan(32).ValueOrDie();
        CHECK_EQ(span->n(), size_t{32});
      }
    }));
  }
}

}  // namespace
