#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "testing/testing.h"

namespace {

TEST(BasicTest) {
  constexpr size_t kSize = 49;
  auto f = capsule::NewHeapStorageFactory().ValueOrDie();
  auto s = capsule::Storage::Allocate(f.get(), kSize).ValueOrDie();
  ASSERT_EQ(s->n(), kSize);
  const auto u64 = reinterpret_cast<uint64_t>(s->DataAsPtrTo<uint64_t>());
  ASSERT_EQ(u64 % 8, 0) << "Expecting 8-alignment.";
}

}  // namespace
