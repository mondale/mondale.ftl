#include <cassert>
#include <iostream>
#include <memory>

#include "base/basic_test.h"

namespace {

// Generally prefer to use the BASIC_TEST() macro...
class PassingTest final : public base::testing::BasicTest {
 protected:
  void Run() final { std::cout << "I am a passing test!"; }
};

}  // namespace

int main(int argc, char* argv[]) {
  base::testing::RegisterTest(std::make_unique<PassingTest>());
  return base::testing::RunAllTests();
}
