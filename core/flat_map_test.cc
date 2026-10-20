#include "core/flat_map.h"
#include "testing/testing.h"

namespace {

using core::FlatMap;

TEST(FlatMapDefaultConstructor) {
  FlatMap<int, std::string> map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

TEST(FlatMapInsertAndFind) {
  FlatMap<int, std::string> map;
  auto [it, inserted] = map.insert({1, "one"});
  EXPECT_TRUE(inserted);
  ASSERT_TRUE(it != map.end());
  EXPECT_EQ(it->first, 1);
  EXPECT_EQ(it->second, "one");

  auto [it2, inserted2] = map.insert({1, "duplicate"});
  EXPECT_FALSE(inserted2);
  EXPECT_EQ(it2->first, 1);
  EXPECT_EQ(it2->second, "one");

  auto found = map.find(1);
  ASSERT_TRUE(found != map.end());
  EXPECT_EQ(found->second, "one");

  auto not_found = map.find(99);
  ASSERT_TRUE(not_found == map.end());
}

TEST(FlatMapElementAccess) {
  FlatMap<std::string, int> map;
  map["apple"] = 10;
  EXPECT_EQ(map["apple"], 10);

  map["apple"] = 20;
  EXPECT_EQ(map["apple"], 20);

  EXPECT_EQ(map.at("apple"), 20);
}

TEST(FlatMapEraseAndClear) {
  FlatMap<int, int> map = {{1, 10}, {2, 20}, {3, 30}};
  EXPECT_EQ(map.size(), 3);

  size_t erased = map.erase(2);
  EXPECT_EQ(erased, 1);
  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains(2));

  size_t erased_missing = map.erase(99);
  EXPECT_EQ(erased_missing, 0);

  map.clear();
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

TEST(FlatMapBoundsAndEqualRange) {
  FlatMap<int, int> map = {{10, 1}, {20, 2}, {30, 3}, {40, 4}};

  auto lb = map.lower_bound(25);
  ASSERT_TRUE(lb != map.end());
  EXPECT_EQ(lb->first, 30);

  auto ub = map.upper_bound(20);
  ASSERT_TRUE(ub != map.end());
  EXPECT_EQ(ub->first, 30);

  auto range = map.equal_range(20);
  ASSERT_TRUE(range.first != map.end());
  EXPECT_EQ(range.first->first, 20);
  ASSERT_TRUE(range.second != map.end());
  EXPECT_EQ(range.second->first, 30);
}

TEST(FlatMapComparison) {
  FlatMap<int, int> map1 = {{1, 10}, {2, 20}};
  FlatMap<int, int> map2 = {{1, 10}, {2, 20}};
  FlatMap<int, int> map3 = {{1, 10}, {2, 25}};

  EXPECT_TRUE(map1 == map2);
  EXPECT_FALSE(map1 == map3);
  EXPECT_TRUE((map1 <=> map3) == std::strong_ordering::less);
}

}  // namespace
