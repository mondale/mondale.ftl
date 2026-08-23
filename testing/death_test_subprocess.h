#ifndef TESTING_DEATH_TEST_SUBPROCESS_H_
#define TESTING_DEATH_TEST_SUBPROCESS_H_

#include <functional>

#include "testing/death.h"

namespace testing::internal {

class DeathTestSubprocess final {
 public:
  static DeathTestResult Execute(std::function<void()> statement);
};

}  // namespace testing::internal

#endif  // #ifndef TESTING_DEATH_TEST_SUBPROCESS_H_
