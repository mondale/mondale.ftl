#ifndef BASE_BASIC_TEST_H_
#define BASE_BASIC_TEST_H_

#include <list>
#include <memory>
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
  void ExpectFailure() { expect_passing_ = false; }

 private:
  std::list<std::string> outs_;
  bool passing_ = true;
  bool expect_passing_ = true;
};

bool RegisterTest(std::unique_ptr<BasicTest> test);
[[nodiscard]] int RunAllTests();

#define BASIC_TEST(name)                                 \
  class name final : public ::base::testing::BasicTest { \
   protected:                                            \
    void Run() final;                                    \
  };                                                     \
  static bool global_##name##_registered =               \
      RegisterTest(std::make_unique<name>());            \
  void name::Run()

}  // namespace base::testing

#endif  // #ifndef BASE_BASIC_TEST_H_
