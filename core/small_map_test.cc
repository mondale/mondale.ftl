#include "core/small_map.h"
#include "testing/testing.h"

namespace {

using core::SmallMap;

TEST(SmallMapDefaultConstruction) {
  SmallMap<int, std::string> map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_GE(map.capacity(), 1);
}

TEST(SmallMapInsertionAndLookup) {
  SmallMap<int, std::string> map;

  auto [it, inserted] = map.insert({1, "one"});
  EXPECT_TRUE(inserted);
  EXPECT_EQ(it->first, 1);
  EXPECT_EQ(it->second, "one");
  EXPECT_EQ(map.size(), 1);

  auto [it2, inserted2] = map.insert({1, "duplicate"});
  EXPECT_FALSE(inserted2);
  EXPECT_EQ(it2->second, "one");

  auto found = map.find(1);
  ASSERT_FALSE(found == map.end());
  EXPECT_EQ(found->second, "one");

  auto not_found = map.find(99);
  EXPECT_TRUE(not_found == map.end());
}

TEST(SmallMapSubscriptAndAt) {
  SmallMap<std::string, int> map;

  map["alpha"] = 10;
  map["beta"] = 20;

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map["alpha"], 10);
  EXPECT_EQ(map["beta"], 20);

  EXPECT_EQ(map.at("alpha"), 10);

  map.at("beta") = 25;
  EXPECT_EQ(map.at("beta"), 25);

  EXPECT_TRUE(map.contains("alpha"));
  EXPECT_TRUE(map.contains("beta"));
  EXPECT_FALSE(map.contains("gamma"));
}

TEST(SmallMapTryEmplace) {
  SmallMap<int, std::string> map;

  auto [it1, ins1] = map.try_emplace(1, "first");
  EXPECT_TRUE(ins1);
  EXPECT_EQ(it1->second, "first");

  auto [it2, ins2] = map.try_emplace(1, "second");
  EXPECT_FALSE(ins2);
  EXPECT_EQ(it2->second, "first");
}

TEST(SmallMapErasure) {
  SmallMap<int, int, 4> map;
  map.insert({10, 100});
  map.insert({20, 200});
  map.insert({30, 300});

  EXPECT_EQ(map.size(), 3);

  size_t erased_count = map.erase(20);
  EXPECT_EQ(erased_count, 1);
  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains(20));
  EXPECT_TRUE(map.contains(10));
  EXPECT_TRUE(map.contains(30));

  map.clear();
  EXPECT_TRUE(map.empty());
}

TEST(SmallMapBoundsAndRange) {
  SmallMap<int, int> map;
  map.insert({5, 50});
  map.insert({1, 10});
  map.insert({9, 90});

  auto lb = map.lower_bound(5);
  ASSERT_FALSE(lb == map.end());
  EXPECT_EQ(lb->first, 5);

  auto ub = map.upper_bound(5);
  ASSERT_FALSE(ub == map.end());
  EXPECT_EQ(ub->first, 9);

  auto range = map.equal_range(5);
  ASSERT_FALSE(range.first == map.end());
  EXPECT_EQ(range.first->first, 5);
  EXPECT_EQ(range.second - range.first, 1);
}

}  // namespace
