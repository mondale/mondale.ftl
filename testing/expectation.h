#ifndef TESTING_EXPECTATION_H_
#define TESTING_EXPECTATION_H_

#include <list>
#include <string>

#include "testing/compare.h"

namespace testing::internal {

class ExpectationResult {
 public:
  ExpectationResult(bool success, std::list<std::string>* outs)
      : success_(success), outs_(outs) {}
  ~ExpectationResult() {
    if (!success_) outs_->push_back(ss_.str());
  }

  // Allows the result object to evaluate directly in 'if' statements
  explicit operator bool() const { return success_; }

  // Stream operator captures custom user message if the assertion failed
  template <typename T>
  ExpectationResult& operator<<(const T& val) {
    if (!success_) {
      ss_ << val;
    }
    return *this;
  }

  ExpectationResult(ExpectationResult&& other) = default;
  ExpectationResult& operator=(ExpectationResult&& other) = default;

 private:
  bool success_;
  std::stringstream ss_;
  std::list<std::string>* outs_;
};

class Expectation {
 public:
  virtual ~Expectation() = default;

  bool IsPassing() const { return expect_passing_ == passing_; }
  const std::list<std::string>& outs() const { return outs_; }

  ExpectationResult AddFailure(const char* file, int line, std::string message);

  ExpectationResult ExpectTrue(const char* file, int line, const char* name,
                               bool value) {
    if (value) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, name, "true", " is ", value, true);
  }

  ExpectationResult ExpectFalse(const char* file, int line, const char* name,
                                bool value) {
    if (!value) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, name, "false", " is ", value, false);
  }

  template <typename L, typename R>
  ExpectationResult ExpectEq(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Eq(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " == ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectLe(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Le(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " <= ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectLt(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Lt(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " < ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectGe(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Ge(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " >= ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectGt(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Gt(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " > ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectNe(const char* file, int line, const char* a_name,
                             const char* b_name, L a, R b) {
    if (Compare::Ne(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " != ", a, b);
  }

  template <typename L, typename R>
  ExpectationResult ExpectNear(const char* file, int line, const char* a_name,
                               const char* b_name, L a, R b) {
    if (Compare::Near(a, b)) return ExpectationResult(true, nullptr);
    return FailExpectation(file, line, a_name, b_name, " near ", a, b);
  }

  template <typename L, typename R, typename U>
  ExpectationResult ExpectNear(const char* file, int line, const char* a_name,
                               const char* b_name, const char* u_name, L a, R b,
                               U u) {
    if (Compare::Near(a, b, u)) return ExpectationResult(true, nullptr);
    std::stringstream ss;
    ss << " within +/-" << u_name << " (" << u << ") of ";
    std::string s = ss.str();
    return FailExpectation(file, line, a_name, b_name, s.c_str(), a, b);
  }

  void ExpectFailure() { expect_passing_ = false; }
  void RestorePassing();

 private:
  template <typename L, typename R>
  ExpectationResult FailExpectation(const char* file, int line,
                                    const char* a_name, const char* b_name,
                                    const char* op, L a, R b) {
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

#endif  // #ifndef TESTING_EXPECTATION_H_
