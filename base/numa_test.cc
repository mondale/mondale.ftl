#include <utility>

#include "base/cpu.h"
#include "base/numa.h"
#include "testing/testing.h"

namespace {

struct TestStruct {
  int a;
  double b;
  bool constructed = false;

  TestStruct(int x, double y) : a(x), b(y), constructed(true) {}
};

struct MoveOnlyStruct {
  int val;
  explicit MoveOnlyStruct(int v) : val(v) {}
  MoveOnlyStruct(const MoveOnlyStruct&) = delete;
  MoveOnlyStruct& operator=(const MoveOnlyStruct&) = delete;
  MoveOnlyStruct(MoveOnlyStruct&&) = default;
  MoveOnlyStruct& operator=(MoveOnlyStruct&&) = default;
};

TEST(NumaAllocatorCurrentNode) {
  auto ptr = base::MakeUniqueNumaAware<TestStruct>(42, 3.14);
  ASSERT_NE(ptr, nullptr);
  EXPECT_TRUE(ptr->constructed);
  EXPECT_EQ(ptr->a, 42);
  EXPECT_EQ(ptr->b, 3.14);
}

TEST(NumaAllocatorExplicitNode) {
  int target_node = base::NumaDomainFor(base::CurrentCpu());
  if (target_node < 0) {
    target_node = 0;
  }

  auto ptr =
      base::MakeUniqueNumaAwareOnNode<TestStruct>(target_node, 100, 2.718);
  ASSERT_NE(ptr, nullptr);
  EXPECT_TRUE(ptr->constructed);
  EXPECT_EQ(ptr->a, 100);
  EXPECT_EQ(ptr->b, 2.718);
}

TEST(NumaAllocatorMoveOnlyType) {
  auto ptr = base::MakeUniqueNumaAware<MoveOnlyStruct>(77);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ptr->val, 77);
}

TEST(NumaAllocatorMoveSemantics) {
  auto ptr1 = base::MakeUniqueNumaAware<TestStruct>(1, 2.0);
  ASSERT_NE(ptr1, nullptr);

  base::NumaUniquePtr<TestStruct> ptr2 = std::move(ptr1);
  EXPECT_EQ(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  EXPECT_EQ(ptr2->a, 1);
}

struct DummyBuffer {
  char data[4096];
};

TEST(NumaMemoryLocalityTest) {
  int total_nodes = base::NumNumaDomains();
  int target_node = base::NumaDomainFor(base::CurrentCpu());
  if (target_node < 0) {
    target_node = 0;
  }

  // Allocate object on the target node
  auto buf = base::MakeUniqueNumaAwareOnNode<DummyBuffer>(target_node);
  ASSERT_NE(buf, nullptr);

  // Touch the memory to ensure physical pages are mapped by the kernel
  buf->data[0] = 'x';
  buf->data[4095] = 'y';

  // Verify the page is backed by the requested NUMA node
  int actual_node = base::GetAddressNumaNode(buf.get());
  EXPECT_EQ(actual_node, target_node);

  // If the machine has multiple NUMA domains, test allocation on a second node
  if (total_nodes > 1) {
    int other_node = (target_node + 1) % total_nodes;
    auto buf_other = base::MakeUniqueNumaAwareOnNode<DummyBuffer>(other_node);
    ASSERT_NE(buf_other, nullptr);

    buf_other->data[0] = 'z';
    int actual_other_node = base::GetAddressNumaNode(buf_other.get());
    EXPECT_EQ(actual_other_node, other_node);
  }
}

}  // namespace
