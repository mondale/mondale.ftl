#include <string>

#include "base/flags.h"
#include "testing/testing.h"

FLAG_COHORT(test_vibes);
FLAG(int, level, 50).Ge(0).Lt(100);
FLAG(std::string, title, "default");

namespace {

TEST(FlagLookupAndDefaults) {
  EXPECT_EQ(FLAG_LOOKUP(level), 50);
  EXPECT_EQ(FLAG_LOOKUP(title), "default");
}

}  // namespace
