#include "core/handle_table.h"
#include "testing/testing.h"

using core::HandleTable;
using testing::IsOk;

namespace {

struct TestObject {
  int value{42};
};

TEST(HandleTableTest_BasicAllocateAndLookup) {
  HandleTable<TestObject, 10> table;

  auto handle = table.Allocate().ValueOrDie();
  auto obj = table.Lookup(handle).ValueOrDie();
  EXPECT_EQ(obj->value, 42);
}

TEST(HandleTableTest_FreeAndReuse) {
  HandleTable<TestObject, 4> table;

  auto h1 = table.Allocate().ValueOrDie();

  auto obj1 = table.Lookup(h1).ValueOrDie();
  obj1->value = 100;

  EXPECT_THAT(table.Free(h1), IsOk());
  EXPECT_FALSE(table.Lookup(h1).IsOk());

  auto h2 = table.Allocate().ValueOrDie();
  EXPECT_NE(h1.value(), h2.value());

  auto obj2 = table.Lookup(h2).ValueOrDie();
  EXPECT_EQ(obj2->value, 42);
}

TEST(HandleTableTest_Exhaustion) {
  HandleTable<TestObject, 2> table;

  static_cast<void>(table.Allocate().ValueOrDie());
  static_cast<void>(table.Allocate().ValueOrDie());

  auto h3_res = table.Allocate();
  EXPECT_FALSE(h3_res.IsOk());
  EXPECT_EQ(h3_res.result().code(), Code::kExhausted);
}

TEST(HandleTableTest_InvalidHandles) {
  HandleTable<TestObject, 10> table;

  auto h1 = table.Allocate().ValueOrDie();
  EXPECT_THAT(table.Free(h1), IsOk());

  EXPECT_FALSE(table.Lookup(h1).IsOk());
  EXPECT_FALSE(table.Free(h1).IsOk());

  using HandleType = HandleTable<TestObject, 10>::Handle;
  HandleType bogus(99999);
  EXPECT_FALSE(table.Lookup(bogus).IsOk());
  EXPECT_FALSE(table.Free(bogus).IsOk());
}

TEST(HandleTableTest_LargeStorageMadvise) {
  struct LargeObject {
    char data[1024 * 1024];  // 1 MiB per object
  };

  // 3 slots total = ~3 MiB, pushing the high slots past the 2 MiB madvise
  // threshold.
  HandleTable<LargeObject, 3> table;

  // Exhaust/advance past the first 2 MiB
  auto h1 = table.Allocate().ValueOrDie();
  auto h2 = table.Allocate().ValueOrDie();
  auto h3 = table.Allocate().ValueOrDie();

  // Access the third slot, which resides in the madvised region (> 2 MiB)
  auto ptr = table.Lookup(h3).ValueOrDie();
  ptr->data[0] = 'a';
  ptr->data[1024 * 1024 - 1] = 'z';

  EXPECT_EQ(ptr->data[0], 'a');
  EXPECT_EQ(ptr->data[1024 * 1024 - 1], 'z');

  EXPECT_THAT(table.Free(h1), IsOk());
  EXPECT_THAT(table.Free(h2), IsOk());
  EXPECT_THAT(table.Free(h3), IsOk());
}

}  // namespace
