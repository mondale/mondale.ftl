#ifndef BASE_BASIC_TEST_H_
#define BASE_BASIC_TEST_H_

#include <memory>

namespace base::testing {

// Dies unceremoniusly with a nonzero exit code.
void Die();

// Writes "SUCCESS" to the file specfied by $RESULTS_FILE or dies trying.
void WriteSuccessResultOrDie();

class BasicTest {
 public:
  virtual ~BasicTest() {}

  virtual void Run() = 0;

 private:
};

void RegisterTest(std::unique_ptr<BasicTest> test);
[[nodiscard]] int RunAllTests();

}  // namespace base::testing

#endif  // #ifndef BASE_BASIC_TEST_H_
