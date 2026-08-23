#ifndef TESTING_MATCHERS_H_
#define TESTING_MATCHERS_H_

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace testing {

// Captures whether a match succeeded and any failure explanations
struct MatchResult {
  bool matched;
  std::string explanation;

  explicit operator bool() const { return matched; }
};

// General signature expectation/concept pattern for matchers
template <typename T>
class Matcher {
 public:
  virtual MatchResult Match(const T& actual) const = 0;
  virtual void DescribeTo(std::ostream& os) const = 0;
};

// String Contains Matcher
class HasSubstrMatcher final {
 public:
  explicit HasSubstrMatcher(std::string substring)
      : substring_(std::move(substring)) {}

  MatchResult Match(const std::string& actual) const;
  void DescribeTo(std::ostream& os) const;

 private:
  std::string substring_;
};

inline auto HasSubstr(std::string substring) {
  return HasSubstrMatcher(std::move(substring));
}

}  // namespace testing

#endif  // #ifndef TESTING_MATCHERS_H_
