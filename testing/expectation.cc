#include <sstream>

#include "testing/death_test_subprocess.h"
#include "testing/expectation.h"

namespace testing::internal {

ExpectationResult Expectation::AddFailure(const char* file, int line,
                                          std::string message) {
  ExpectationResult r(false, &outs_);
  passing_ = false;
  r << file << "[" << line << "]: " << message;
  return r;
}

void Expectation::RestorePassing() {
  passing_ = true;
  expect_passing_ = true;
  outs_.clear();
}

ExpectationResult Expectation::ExpectDeath(const char* file, int line,
                                           std::function<void()> expression,
                                           DeathMatcher m) {
  const auto result = DeathTestSubprocess::Execute(expression);
  std::string complaint;
  if (m(result, complaint)) return ExpectationResult(true, nullptr);
  return AddFailure(file, line, complaint);
}

}  // namespace testing::internal
