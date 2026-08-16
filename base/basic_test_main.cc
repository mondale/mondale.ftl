#include "base/basic_test.h"
#include "base/process.h"

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  return base::testing::RunAllTests();
}
