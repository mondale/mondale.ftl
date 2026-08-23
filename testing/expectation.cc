#include <sstream>

#include "testing/expectation.h"

namespace testing::internal {

void Expectation::AddFailure(const char* file, int line, std::string message) {
  passing_ = false;
  std::stringstream ss;
  ss << file << "[" << line << "]: " << message;
  outs_.push_back(ss.str());
}

}  // namespace testing::internal
