#ifndef BASE_BASIC_TEST_H_
#define BASE_BASIC_TEST_H_

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

namespace base::testing {

class BasicTest {
 public:
  virtual ~BasicTest() {}

  virtual void Run() = 0;

  // Returns the name of the final derived class.
  std::string GetName() const;

  bool IsPassing() const { return expect_passing_ == passing_; }
  const std::list<std::string>& outs() const { return outs_; }

 protected:
  void AddFailure(const char* file, int line, std::string message);

  template <typename T>
  void AssertEqHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a == b) return;
    FailExpectation(file, line, a_name, b_name, " == ", a, b);
    Assert();
  }

  template <typename T>
  void AssertLeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a <= b) return;
    FailExpectation(file, line, a_name, b_name, " <= ", a, b);
    Assert();
  }

  template <typename T>
  void AssertLtHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a < b) return;
    FailExpectation(file, line, a_name, b_name, " < ", a, b);
    Assert();
  }

  template <typename T>
  void AssertGeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a >= b) return;
    FailExpectation(file, line, a_name, b_name, " >= ", a, b);
    Assert();
  }

  template <typename T>
  void AssertGtHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a > b) return;
    FailExpectation(file, line, a_name, b_name, " > ", a, b);
    Assert();
  }

  template <typename T>
  void AssertNeHelper(const char* file, int line, const char* a_name,
                      const char* b_name, T a, T b) {
    if (a != b) return;
    FailExpectation(file, line, a_name, b_name, " != ", a, b);
    Assert();
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
  void ExpectAssert() {
    ExpectFailure();
    expect_assert_ = true;
  }

 private:
  void Assert() {
    for (const auto& finding : outs_) {
      std::cerr << finding << std::endl;
    }
    std::cerr << "Assertion failed, aborting test." << std::endl;
    std::cout << std::flush;
    if (expect_assert_) return;
    exit(1);
  }

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
  bool expect_assert_ = false;
};

[[nodiscard]] bool RegisterTest(std::unique_ptr<BasicTest> test);
[[nodiscard]] int RunAllTests();

#define TEST(name)                                       \
  class name final : public ::base::testing::BasicTest { \
   protected:                                            \
    void Run() final;                                    \
  };                                                     \
  static bool global_##name##_registered =               \
      RegisterTest(std::make_unique<name>());            \
  void name::Run()

#define ADD_FAILURE(msg) AddFailure(__FILE__, __LINE__, msg)

#define EXPECT_EQ(a, b) ExpectEqHelper(__FILE__, __LINE__, #a, #b, a, b)
#define EXPECT_NE(a, b) ExpectNeHelper(__FILE__, __LINE__, #a, #b, a, b)
#define EXPECT_LT(a, b) ExpectLtHelper(__FILE__, __LINE__, #a, #b, a, b)
#define EXPECT_LE(a, b) ExpectLeHelper(__FILE__, __LINE__, #a, #b, a, b)
#define EXPECT_GT(a, b) ExpectGtHelper(__FILE__, __LINE__, #a, #b, a, b)
#define EXPECT_GE(a, b) ExpectGeHelper(__FILE__, __LINE__, #a, #b, a, b)

#define ASSERT_EQ(a, b) AssertEqHelper(__FILE__, __LINE__, #a, #b, a, b)
#define ASSERT_NE(a, b) AssertNeHelper(__FILE__, __LINE__, #a, #b, a, b)
#define ASSERT_LT(a, b) AssertLtHelper(__FILE__, __LINE__, #a, #b, a, b)
#define ASSERT_LE(a, b) AssertLeHelper(__FILE__, __LINE__, #a, #b, a, b)
#define ASSERT_GT(a, b) AssertGtHelper(__FILE__, __LINE__, #a, #b, a, b)
#define ASSERT_GE(a, b) AssertGeHelper(__FILE__, __LINE__, #a, #b, a, b)

}  // namespace base::testing

#endif  // #ifndef BASE_BASIC_TEST_H_
