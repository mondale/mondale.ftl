#include "testing/matchers.h"

namespace testing {

MatchResult HasSubstrMatcher::Match(const std::string& actual) const {
  if (actual.find(substring_) != std::string::npos) {
    return {true, ""};
  }
  std::ostringstream ss;
  ss << actual;
  return {false, ss.str()};
}

void HasSubstrMatcher::DescribeTo(std::ostream& os) const {
  os << "contains substring [" << substring_ << "]";
}

void IsOkMatcher::DescribeTo(std::ostream& os) const {
  os << "contains an OK Result or Code.";
}

}  // namespace testing
