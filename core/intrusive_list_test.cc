#include "core/intrusive_list.h"
#include "testing/testing.h"

namespace {

struct TestItem : public core::IntrusiveListHook<void> {
  int value;
  explicit TestItem(int v) : value(v) {}
};

struct MultiTestItem : public core::IntrusiveListHook<struct TagA>,
                       public core::IntrusiveListHook<struct TagB> {
  int value;
  explicit MultiTestItem(int v) : value(v) {}
};

TEST(IntrusiveListTest_DefaultConstructedIsEmpty) {
  core::IntrusiveList<TestItem> list;
  EXPECT_TRUE(list.Empty());
  EXPECT_TRUE(list.begin() == list.end());
}

TEST(IntrusiveListTest_PushBackAndIterate) {
  core::IntrusiveList<TestItem> list;
  TestItem a(1);
  TestItem b(2);
  TestItem c(3);

  EXPECT_FALSE(list.IsLinked(&a));

  list.PushBack(&a);
  list.PushBack(&b);
  list.PushBack(&c);

  EXPECT_TRUE(list.IsLinked(&a));
  EXPECT_FALSE(list.Empty());

  int expected = 1;
  for (const auto& item : list) {
    EXPECT_EQ(item.value, expected);
    ++expected;
  }
  EXPECT_EQ(expected, 4);
}

TEST(IntrusiveListTest_PushFrontAndIterate) {
  core::IntrusiveList<TestItem> list;
  TestItem a(1);
  TestItem b(2);
  TestItem c(3);

  list.PushFront(&a);
  list.PushFront(&b);
  list.PushFront(&c);

  int expected_values[] = {3, 2, 1};
  int i = 0;
  for (const auto& item : list) {
    EXPECT_EQ(item.value, expected_values[i]);
    ++i;
  }
  EXPECT_EQ(i, 3);
}

TEST(IntrusiveListTest_PopFrontAndPopBack) {
  core::IntrusiveList<TestItem> list;
  TestItem a(1);
  TestItem b(2);
  TestItem c(3);

  list.PushBack(&a);
  list.PushBack(&b);
  list.PushBack(&c);

  list.PopFront();
  EXPECT_FALSE(list.IsLinked(&a));
  EXPECT_EQ(list.begin()->value, 2);

  list.PopBack();
  EXPECT_FALSE(list.IsLinked(&c));
  EXPECT_EQ(list.begin()->value, 2);

  list.PopBack();
  EXPECT_FALSE(list.IsLinked(&b));
  EXPECT_TRUE(list.Empty());
}

TEST(IntrusiveListTest_ClearUnlinksElements) {
  core::IntrusiveList<TestItem> list;
  TestItem a(1);
  TestItem b(2);

  list.PushBack(&a);
  list.PushBack(&b);

  list.Clear();
  EXPECT_TRUE(list.Empty());
  EXPECT_FALSE(list.IsLinked(&a));
  EXPECT_FALSE(list.IsLinked(&b));
}

TEST(IntrusiveListTest_MultiListMembership) {
  core::IntrusiveList<MultiTestItem, TagA> list_a;
  core::IntrusiveList<MultiTestItem, TagB> list_b;

  MultiTestItem item(42);

  list_a.PushBack(&item);
  list_b.PushBack(&item);

  EXPECT_TRUE(list_a.IsLinked(&item));
  EXPECT_TRUE(list_b.IsLinked(&item));

  EXPECT_FALSE(list_a.Empty());
  EXPECT_FALSE(list_b.Empty());

  EXPECT_EQ(list_a.begin()->value, 42);
  EXPECT_EQ(list_b.begin()->value, 42);

  list_a.Clear();
  EXPECT_TRUE(list_a.Empty());
  EXPECT_FALSE(list_b.Empty());
  EXPECT_FALSE(list_a.IsLinked(&item));
  EXPECT_TRUE(list_b.IsLinked(&item));

  list_b.Clear();
  EXPECT_TRUE(list_b.Empty());
  EXPECT_FALSE(list_b.IsLinked(&item));
}

TEST(IntrusiveListTest_BlindErase) {
  core::IntrusiveList<TestItem> list;
  TestItem a(10);
  TestItem b(20);

  list.PushBack(&a);
  list.PushBack(&b);

  core::IntrusiveList<TestItem>::Erase(&a);

  EXPECT_FALSE(list.IsLinked(&a));
  EXPECT_FALSE(list.Empty());
  EXPECT_EQ(list.begin()->value, 20);

  core::IntrusiveList<TestItem>::Erase(&b);
  EXPECT_TRUE(list.Empty());
}

TEST(IntrusiveListTest_MoveSemantics) {
  core::IntrusiveList<TestItem> list1;
  TestItem a(1);
  TestItem b(2);

  list1.PushBack(&a);
  list1.PushBack(&b);

  core::IntrusiveList<TestItem> list2(std::move(list1));
  EXPECT_TRUE(list1.Empty());
  EXPECT_FALSE(list2.Empty());

  int expected = 1;
  for (const auto& item : list2) {
    EXPECT_EQ(item.value, expected);
    ++expected;
  }

  core::IntrusiveList<TestItem> list3;
  list3 = std::move(list2);
  EXPECT_TRUE(list2.Empty());
  EXPECT_FALSE(list3.Empty());

  expected = 1;
  for (const auto& item : list3) {
    EXPECT_EQ(item.value, expected);
    ++expected;
  }
}

}  // namespace
