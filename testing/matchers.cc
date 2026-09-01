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

MatchResult StrEqMatcher::Match(std::string_view actual) const {
  return Match(std::string(actual));
}

MatchResult StrEqMatcher::Match(const std::string& actual) const {
  if (actual == s_) return {true, ""};
  std::ostringstream ss;
  ss << actual;
  return {false, ss.str()};
}

void StrEqMatcher::DescribeTo(std::ostream& os) const {
  os << "exactly matches string [" << s_ << "]";
}

void IsOkMatcher::DescribeTo(std::ostream& os) const {
  os << "contains an OK Result or Code.";
}

}  // namespace testing
