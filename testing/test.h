#ifndef TESTING_TEST_H_
#define TESTING_TEST_H_

#include <signal.h>

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

#include "testing/expectation.h"

namespace testing {

class Test : public internal::Expectation {
 public:
  virtual ~Test() = default;

  // Returns the name of the final derived class.
  std::string GetName() const;

  static Test* Current();

  virtual void SetUp() {}
  virtual void TearDown() {}
  virtual void TestBody() = 0;

 private:
};

namespace internal {

// Abstract factory interface to instantiate tests on demand
class TestFactory {
 public:
  virtual ~TestFactory() = default;
  virtual Test* CreateTest() = 0;
};

// Registry tracking tests across translation units
class TestRegistry {
 public:
  static TestRegistry* Instance();

  void RegisterTest(const std::string& suite_name, const std::string& test_name,
                    std::unique_ptr<TestFactory> factory);

  int RunAllTests();

 private:
  struct TestInfo {
    std::string suite_name;
    std::string test_name;
    std::unique_ptr<TestFactory> factory;
  };
  std::list<TestInfo> tests_;
};

// Helper struct for static registration before main() runs
struct TestRegistrar {
  TestRegistrar(const char* suite_name, const char* test_name,
                std::unique_ptr<TestFactory> factory);
};

}  // namespace internal

#define TEST_F_CONCAT_IMPL(a, b) a##_##b##_Test
#define TEST_F_CONCAT(a, b) TEST_F_CONCAT_IMPL(a, b)

#define TEST_F(FixtureClass, TestName)                              \
  class FixtureClass##_##TestName##_Test : public FixtureClass {    \
   public:                                                          \
    FixtureClass##_##TestName##_Test() = default;                   \
    void TestBody() override;                                       \
  };                                                                \
  class FixtureClass##_##TestName##_Factory                         \
      : public ::testing::internal::TestFactory {                   \
   public:                                                          \
    ::testing::Test* CreateTest() override {                        \
      return new FixtureClass##_##TestName##_Test();                \
    }                                                               \
  };                                                                \
  static ::testing::internal::TestRegistrar                         \
      FixtureClass##_##TestName##_registrar(                        \
          #FixtureClass, #TestName,                                 \
          std::make_unique<FixtureClass##_##TestName##_Factory>()); \
  void FixtureClass##_##TestName##_Test::TestBody()

#define TEST_IMPL_SINGLE_(FixtureClass, TestName)                            \
  class TestName##_Test : public FixtureClass {                              \
   public:                                                                   \
    TestName##_Test() = default;                                             \
    void TestBody() override;                                                \
  };                                                                         \
  class TestName##_Test_Factory : public ::testing::internal::TestFactory {  \
   public:                                                                   \
    ::testing::Test* CreateTest() override { return new TestName##_Test(); } \
  };                                                                         \
  static ::testing::internal::TestRegistrar TestName##_Test_registrar(       \
      __FILE__, #TestName, std::make_unique<TestName##_Test_Factory>());     \
  void TestName##_Test::TestBody()

#define TEST(TestName) TEST_IMPL_SINGLE_(::testing::Test, TestName)

#define ADD_FAILURE(msg) AddFailure(__FILE__, __LINE__, msg)

#define EXPECT_TRUE(a) \
  (::testing::Test::Current()->ExpectTrue(__FILE__, __LINE__, #a, a))
#define EXPECT_FALSE(a) \
  (::testing::Test::Current()->ExpectFalse(__FILE__, __LINE__, #a, a))
#define EXPECT_EQ(a, b) \
  (::testing::Test::Current()->ExpectEq(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_NE(a, b) \
  (::testing::Test::Current()->ExpectNe(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_LT(a, b) \
  (::testing::Test::Current()->ExpectLt(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_LE(a, b) \
  (::testing::Test::Current()->ExpectLe(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_GT(a, b) \
  (::testing::Test::Current()->ExpectGt(__FILE__, __LINE__, #a, #b, a, b))
#define EXPECT_GE(a, b) \
  (::testing::Test::Current()->ExpectGe(__FILE__, __LINE__, #a, #b, a, b))

#define ASSERT_TRUE(a)                                                 \
  (::testing::Test::Current()->ExpectTrue(__FILE__, __LINE__, #a, a)); \
  return
#define ASSERT_FALSE(a)                                                 \
  (::testing::Test::Current()->ExpectFalse(__FILE__, __LINE__, #a, a)); \
  return
#define ASSERT_EQ(a, b)                                                     \
  (::testing::Test::Current()->ExpectEq(__FILE__, __LINE__, #a, #b, a, b)); \
  return
#define ASSERT_NE(a, b)                                                     \
  (::testing::Test::Current()->ExpectNe(__FILE__, __LINE__, #a, #b, a, b)); \
  return
#define ASSERT_LT(a, b)                                                     \
  (::testing::Test::Current()->ExpectLt(__FILE__, __LINE__, #a, #b, a, b)); \
  return
#define ASSERT_LE(a, b)                                                     \
  (::testing::Test::Current()->ExpectLe(__FILE__, __LINE__, #a, #b, a, b)); \
  return
#define ASSERT_GT(a, b)                                                     \
  (::testing::Test::Current()->ExpectGt(__FILE__, __LINE__, #a, #b, a, b)); \
  return
#define ASSERT_GE(a, b)                                                     \
  (::testing::Test::Current()->ExpectGe(__FILE__, __LINE__, #a, #b, a, b)); \
  return

[[nodiscard]] int RunAllTests();

}  // namespace testing

#endif  // #ifndef TESTING_TEST_H_
