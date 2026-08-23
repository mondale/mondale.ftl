#ifndef TESTING_EXPECTATION_H_
#define TESTING_EXPECTATION_H_

#include <list>
#include <string>

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

  template <typename T>
  void ExpectEqHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a == b) return;
    FailExpectation(file, line, a_name, b_name, " == ", a, b);
  }

  template <typename T>
  void ExpectLeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a <= b) return;
    FailExpectation(file, line, a_name, b_name, " <= ", a, b);
  }

  template <typename T>
  void ExpectLtHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a < b) return;
    FailExpectation(file, line, a_name, b_name, " < ", a, b);
  }

  template <typename T>
  void ExpectGeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a >= b) return;
    FailExpectation(file, line, a_name, b_name, " >= ", a, b);
  }

  template <typename T>
  void ExpectGtHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a > b) return;
    FailExpectation(file, line, a_name, b_name, " > ", a, b);
  }

  template <typename T>
  void ExpectNeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a != b) return;
    FailExpectation(file, line, a_name, b_name, " != ", a, b);
  }

  void ExpectFailure() { expect_passing_ = false; }
  void RestorePassing();

 private:
  template <typename T>
  void FailExpectation(const char* file, int line, const char* a_name,
                       const char* b_name, const char* op, T a, T b) {
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
