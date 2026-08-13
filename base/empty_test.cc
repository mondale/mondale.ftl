#include <cassert>
#include <iostream>
#include <memory>

#include "base/basic_test.h"

namespace {

class TestTest final : public base::testing::BasicTest {
 protected:
  void Run() final { std::cout << "I am a test!"; }
};

}  // namespace

int main(int argc, char* argv[]) {
  base::testing::RegisterTest(std::make_unique<TestTest>());
  return base::testing::RunAllTests();
}
