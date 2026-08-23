#include <sstream>

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

}  // namespace testing::internal
