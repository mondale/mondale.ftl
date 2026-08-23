#ifndef TESTING_TEST_H_
#define TESTING_TEST_H_

#include <signal.h>

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

namespace testing {

class Test {
 public:
  virtual ~Test() {}

  virtual void Run() = 0;

  // Returns the name of the final derived class.
  std::string GetName() const;

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

  static Test* Current();

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

[[nodiscard]] bool RegisterTest(std::unique_ptr<Test> test);
[[nodiscard]] int RunAllTests();

#define TEST(name)                            \
  class name final : public ::testing::Test { \
   protected:                                 \
    void Run() final;                         \
  };                                          \
  static bool global_##name##_registered =    \
      RegisterTest(std::make_unique<name>()); \
  void name::Run()

#define ADD_FAILURE(msg) AddFailure(__FILE__, __LINE__, msg)

#define EXPECT_TRUE(a) \
  (::testing::Test::Current()->ExpectTrue(__FILE__, __LINE__, #a, a))
#define EXPECT_FALSE(a) \
  (::testing::Test::Current()->ExpectFalse(__FILE__, __LINE__, #a, a))
#define EXPECT_EQ(a, b) \
  (::testing::Test::Current()->ExpectEqHelper(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_NE(a, b) \
  (::testing::Test::Current()->ExpectNeHelper(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_LT(a, b) \
  (::testing::Test::Current()->ExpectLtHelper(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_LE(a, b) \
  (::testing::Test::Current()->ExpectLeHelper(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_GT(a, b) \
  (::testing::Test::Current()->ExpectGtHelper(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_GE(a, b) \
  (::testing::Test::Current()->ExpectGeHelper(__FILE__, __LINE__, #a, #b, a, b))

#define ASSERT_TRUE(a)                                                 \
  (::testing::Test::Current()->ExpectTrue(__FILE__, __LINE__, #a, a)); \
  return
#define ASSERT_FALSE(a)                                                 \
  (::testing::Test::Current()->ExpectFalse(__FILE__, __LINE__, #a, a)); \
  return
#define ASSERT_EQ(a, b)                                                      \
  (::testing::Test::Current()->ExpectEqHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return
#define ASSERT_NE(a, b)                                                      \
  (::testing::Test::Current()->ExpectNeHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return
#define ASSERT_LT(a, b)                                                      \
  (::testing::Test::Current()->ExpectLtHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return
#define ASSERT_LE(a, b)                                                      \
  (::testing::Test::Current()->ExpectLeHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return
#define ASSERT_GT(a, b)                                                      \
  (::testing::Test::Current()->ExpectGtHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return
#define ASSERT_GE(a, b)                                                      \
  (::testing::Test::Current()->ExpectGeHelper(__FILE__, __LINE__, #a, #b, a, \
                                              b));                           \
  return

}  // namespace testing

#endif  // #ifndef TESTING_TEST_H_
