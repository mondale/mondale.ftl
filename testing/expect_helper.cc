#include <sstream>

#include "testing/death_test_subprocess.h"
#include "testing/expect_helper.h"

namespace testing::internal {

ExpectResult ExpectHelper::AddFailure(const char* file, int line,
                                      std::string message) {
  ExpectResult r(false, &outs_);
  passing_ = false;
  r << file << "[" << line << "]: " << message;
  return r;
}

void ExpectHelper::RestorePassing() {
  passing_ = true;
  expect_passing_ = true;
  outs_.clear();
}

ExpectResult ExpectHelper::ExpectDeath(const char* file, int line,
                                       std::function<void()> expression,
                                       DeathMatcher m) {
  const auto result = DeathTestSubprocess::Execute(expression);
  std::string complaint;
  if (m(result, complaint)) return ExpectResult(true, nullptr);
  return AddFailure(file, line, complaint);
}

}  // namespace testing::internal
