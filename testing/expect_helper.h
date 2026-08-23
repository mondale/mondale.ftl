#ifndef TESTING_EXPECT_HELPER_H_
#define TESTING_EXPECT_HELPER_H_

#include <list>
#include <string>

#include "testing/compare.h"
#include "testing/death.h"

namespace testing::internal {

class ExpectResult {
 public:
  ExpectResult(bool success, std::list<std::string>* outs)
      : success_(success), outs_(outs) {}
  ~ExpectResult() {
    if (!success_) outs_->push_back(ss_.str());
  }

  // Allows the result object to evaluate directly in 'if' statements
  explicit operator bool() const { return success_; }

  // Stream operator captures custom user message if the assertion failed
  template <typename T>
  ExpectResult& operator<<(const T& val) {
    if (!success_) {
      ss_ << val;
    }
    return *this;
  }

  ExpectResult(ExpectResult&& other) = default;
  ExpectResult& operator=(ExpectResult&& other) = default;

  int LoopingHelper() {
    if (success_) {
      return 9;
    }
    if (looping_helper_ >= 1) {
      return 1;
    }
    return looping_helper_++;
  }

 private:
  bool success_;
  std::stringstream ss_;
  std::list<std::string>* outs_;
  int looping_helper_ = 0;
};

class ExpectHelper {
 public:
  virtual ~ExpectHelper() = default;

  bool IsPassing() const { return expect_passing_ == passing_; }
  const std::list<std::string>& outs() const { return outs_; }

  ExpectResult AddFailure(const char* file, int line, std::string message);

  ExpectResult ExpectTrue(const char* file, int line, const char* name,
                          bool value) {
    if (value) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, name, "true", " is ", value, true);
  }

  ExpectResult ExpectFalse(const char* file, int line, const char* name,
                           bool value) {
    if (!value) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, name, "false", " is ", value, false);
  }

  template <typename L, typename R>
  ExpectResult ExpectEq(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Eq(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " == ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectLe(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Le(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " <= ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectLt(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Lt(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " < ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectGe(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Ge(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " >= ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectGt(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Gt(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " > ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectNe(const char* file, int line, const char* a_name,
                        const char* b_name, const L& a, const R& b) {
    if (Compare::Ne(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " != ", a, b);
  }

  template <typename L, typename R>
  ExpectResult ExpectNear(const char* file, int line, const char* a_name,
                          const char* b_name, const L& a, const R& b) {
    if (Compare::Near(a, b)) return ExpectResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " near ", a, b);
  }

  template <typename L, typename R, typename U>
  ExpectResult ExpectNear(const char* file, int line, const char* a_name,
                          const char* b_name, const char* u_name, const L& a,
                          const R& b, const U& u) {
    if (Compare::Near(a, b, u)) return ExpectResult(true, nullptr);
    std::stringstream ss;
    ss << " within +/-" << u_name << " (" << u << ") of ";
    std::string s = ss.str();
    return FailExpectation(file, line, a_name, b_name, s.c_str(), a, b);
  }

  ExpectResult ExpectDeath(const char* file, int line,
                           std::function<void()> expression, DeathMatcher m);

  template <typename ValueT, typename MatcherT>
  ExpectResult ExpectThat(const char* file, int line, const char* v_name,
                          const char* m_name, const ValueT& val, MatcherT&& m) {
    const auto mr = m.Match(val);
    if (mr.matched) return ExpectResult(true, nullptr);
    std::stringstream ss;
    ss << "\n  Value of [" << v_name << "]\n";
    ss << "\n  Expected [";
    m.DescribeTo(ss);
    ss << "]\n";
    ss << "\n  Actual [";
    if (mr.explanation.empty()) {
      ss << val;
    } else {
      ss << mr.explanation;
    }
    ss << "]\n";
    return AddFailure(file, line, ss.str());
  }

  void ExpectFailure() { expect_passing_ = false; }
  void RestorePassing();

 private:
  template <typename L, typename R>
  ExpectResult FailExpectation(const char* file, int line, const char* a_name,
                               const char* b_name, const char* op, const L& a,
                               const R& b) {
    std::stringstream ss;
    ss << "\nExpected " << a_name << op << b_name << "\n";
    ss << "        {" << a << "}" << op << "{" << b << "}\n";
    return AddFailure(file, line, ss.str());
  }

  std::list<std::string> outs_;
  bool passing_ = true;
  bool expect_passing_ = true;
};

}  // namespace testing::internal

#endif  // #ifndef TESTING_EXPECT_HELPER_H_
