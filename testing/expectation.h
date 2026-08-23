#ifndef TESTING_EXPECTATION_H_
#define TESTING_EXPECTATION_H_

#include <list>
#include <string>

#include "testing/compare.h"

namespace testing::internal {

class Expectation {
 public:
  virtual ~Expectation() = default;

  bool IsPassing() const { return expect_passing_ == passing_; }
  const std::list<std::string>& outs() const { return outs_; }

  void AddFailure(const char* file, int line, std::string message);

  void ExpectTrue(const char* file, int line, const char* name, bool value) {
    if (value) return;
    FailExpectation(file, line, name, "true", " is ", value, true);
  }

  void ExpectFalse(const char* file, int line, const char* name, bool value) {
    if (!value) return;
    FailExpectation(file, line, name, "false", " is ", value, false);
  }

  template <typename L, typename R>
  void ExpectEq(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Eq(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " == ", a, b);
  }

  template <typename L, typename R>
  void ExpectLe(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Le(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " <= ", a, b);
  }

  template <typename L, typename R>
  void ExpectLt(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Lt(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " < ", a, b);
  }

  template <typename L, typename R>
  void ExpectGe(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Ge(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " >= ", a, b);
  }

  template <typename L, typename R>
  void ExpectGt(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Gt(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " > ", a, b);
  }

  template <typename L, typename R>
  void ExpectNe(const char* file, int line, const char* a_name,
                const char* b_name, L a, R b) {
    if (Compare::Ne(a, b)) return;
    FailExpectation(file, line, a_name, b_name, " != ", a, b);
  }

  void ExpectFailure() { expect_passing_ = false; }
  void RestorePassing();

 private:
  template <typename L, typename R>
  void FailExpectation(const char* file, int line, const char* a_name,
                       const char* b_name, const char* op, L a, R b) {
    std::stringstream ss;
    ss << "\nExpected " << a_name << op << b_name << "\n";
    ss << "        {" << a << "}" << op << "{" << b << "}\n";
    AddFailure(file, line, ss.str());
  }

  std::list<std::string> outs_;
  bool passing_ = true;
  bool expect_passing_ = true;
};

}  // namespace testing::internal

#endif  // #ifndef TESTING_EXPECTATION_H_
