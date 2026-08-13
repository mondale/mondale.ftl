#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include "base/basic_test.h"

namespace base::testing {

void Die() { exit(-1); }

void WriteSuccessResultOrDie() {
  // 1. Fetch the environment variable
  const char* const result_path = std::getenv("RESULTS_FILE");
  if (!result_path) {
    std::cerr << "Error: RESULTS_FILE environment variable is set."
              << std::endl;
    Die();
  }

  // 2. Open the destination file for append.
  std::ofstream file(result_path, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file at path [" << result_path << "]"
              << std::endl;
    Die();
  }

  // 3. Write "SUCCESS" to the file
  file << "SUCCESS" << std::endl;
  if (!file.good()) {
    std::cerr << "Error: File [" << result_path
              << "] not .good() after writing." << std::endl;
    Die();
  }

  file.close();
}

namespace {

std::list<std::unique_ptr<BasicTest>> global_all_tests;

}  // namespace

void RegisterTest(std::unique_ptr<BasicTest> test) {
  global_all_tests.push_back(std::move(test));
}

int RunAllTests() {
  for (auto& test : global_all_tests) {
    test->Run();
  }
  WriteSuccessResultOrDie();
  return EXIT_SUCCESS;
}

}  // namespace base::testing
