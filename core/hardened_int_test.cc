#include <iostream>
#include <sstream>
#include <unordered_set>

#include "core/hardened_int.h"
#include "testing/testing.h"

namespace {

HARDENED_INT_TYPE(Meters, int64_t);
HARDENED_INT_TYPE(Pixels, int64_t);

TEST(HardenedIntOperations) {
  Meters m1{100};
  Meters m2{50};

  // Basic access
  EXPECT_TRUE(m1.value() == 100);

  // Arithmetic
  Meters sum = m1 + m2;
  EXPECT_TRUE(sum.value() == 150);

  sum += Meters{10};
  EXPECT_TRUE(sum.value() == 160);

  // Increment/Decrement
  EXPECT_TRUE((++sum).value() == 161);
  EXPECT_TRUE((sum++).value() == 161);
  EXPECT_TRUE(sum.value() == 162);

  // Ordering & Comparison
  EXPECT_TRUE(m2 < m1);
  EXPECT_TRUE(m1 >= m2);
  EXPECT_TRUE(m1 != m2);
  EXPECT_TRUE(m1 == Meters{100});

  // String formatting
  EXPECT_TRUE(m1.ToString() == "100");

  std::ostringstream ss;
  ss << m1;
  EXPECT_TRUE(ss.str() == "100");
}

TEST(TestHardenedIntHashing) {
  std::unordered_set<Meters> unique_meters;
  unique_meters.insert(Meters{10});
  unique_meters.insert(Meters{20});

  EXPECT_TRUE(unique_meters.contains(Meters{10}));
  EXPECT_TRUE(!unique_meters.contains(Meters{30}));
}

}  // namespace
