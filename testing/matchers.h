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

template <typename InnerMatcher>
class NotMatcher {
 public:
  explicit NotMatcher(InnerMatcher matcher) : matcher_(std::move(matcher)) {}

  template <typename Actual>
  MatchResult Match(const Actual& actual) const {
    MatchResult inner_res = matcher_.Match(actual);
    if (!inner_res.matched) {
      // Inner matcher failed, so NotMatcher succeeds
      return {true, ""};
    }
    // Inner matcher passed, so NotMatcher fails
    std::ostringstream ss;
    ss << "which does match (";
    matcher_.DescribeTo(ss);
    ss << ")";
    return {false, ss.str()};
  }

  void DescribeTo(std::ostream& os) const {
    os << "does not ";
    matcher_.DescribeTo(os);
  }

 private:
  InnerMatcher matcher_;
};

// Factory function template enabling automatic type deduction
template <typename InnerMatcher>
auto Not(InnerMatcher&& matcher) {
  return NotMatcher<std::decay_t<InnerMatcher>>(
      std::forward<InnerMatcher>(matcher));
}

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

// OK matcher.
class IsOkMatcher final {
 public:
  IsOkMatcher() {}

  template <typename T>
  MatchResult Match(const T& t) const {
    if (IsOk(t)) return {true, ""};
    std::ostringstream ss;
    ss << t;
    return {false, ss.str()};
  }

  void DescribeTo(std::ostream& os) const;
};

inline auto IsOk() { return IsOkMatcher(); }

}  // namespace testing

#endif  // #ifndef TESTING_MATCHERS_H_
