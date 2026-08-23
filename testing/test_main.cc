#include "base/process.h"
#include "testing/testing.h"

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  return testing::RunAllTests();
}
