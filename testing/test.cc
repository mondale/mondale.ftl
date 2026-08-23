#include <cxxabi.h>

#include <chrono>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>

#include "testing/test.h"

namespace testing {
namespace {

void Die() { exit(-1); }

std::string MakeSafeDuration(double secs) {
  return std::format("{:.2f}", secs);
}

void AppendResultOrDie(const std::string& name, bool pass,
                       double duration_seconds,
                       const std::list<std::string>& findings) {
  const auto safe_duration = MakeSafeDuration(duration_seconds);

  // 1. Fetch the environment variable
  const char* result_path = std::getenv("RESULTS_FILE");
  if (!result_path) {
    std::cerr
        << "Error: RESULTS_FILE environment variable is not set. Using out.txt"
        << std::endl;
    result_path = "out.txt";
  }

  // 2. Open the destination file for append.
  std::ofstream file(result_path, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file at path [" << result_path << "]"
              << std::endl;
    Die();
  }

  // 3. Write outcome to the file
  file << "=== RUN   " << name << "\n";
  if (pass) {
    file << "--- ";
    file << "PASS";
    file << ": " << name << " (" << safe_duration << "s)\n";
  } else {
    // file << "--- FAIL: " << name << " (0.02s)\n";
    file << "--- FAIL: " << name << " (" << safe_duration << "s)\n";
    for (const auto& finding : findings) {
      file << finding << "\n";
    }
    file << "FAIL\n";
  }

  // Verify file remains valid.
  if (!file.good()) {
    std::cerr << "Error: File [" << result_path
              << "] not .good() after writing." << std::endl;
    Die();
  }

  file.close();
}

std::string Demangle(const char* mangled) {
  int status = 0;
  std::unique_ptr<char, void (*)(void*)> res{
      abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free};
  return (status == 0) ? res.get() : mangled;
}

std::string StripNamespace(const std::string& str) {
  auto pos = str.rfind("::");
  if (pos != std::string_view::npos) {
    return str.substr(pos + 2);
  }
  return str;
}

Test* global_current_test = nullptr;

}  // namespace

std::string Test::GetName() const {
  return StripNamespace(Demangle(typeid(*this).name()));
}

namespace internal {

// static
TestRegistry* TestRegistry::Instance() {
  static TestRegistry* instance = new TestRegistry();
  return instance;
}

void TestRegistry::RegisterTest(const std::string& suite_name,
                                const std::string& test_name,
                                std::unique_ptr<TestFactory> factory) {
  tests_.push_back({suite_name, test_name, std::move(factory)});
}

int TestRegistry::RunAllTests() {
  int failures = 0;
  for (const auto& entry : tests_) {
    std::cout << "[ RUN      ] " << entry.suite_name << "." << entry.test_name
              << "\n";
    std::unique_ptr<Test> test(entry.factory->CreateTest());
    global_current_test = test.get();
    test->SetUp();
    const auto start = std::chrono::steady_clock::now();
    test->TestBody();
    const auto end = std::chrono::steady_clock::now();
    test->TearDown();
    global_current_test = nullptr;
    const std::chrono::duration<double> elapsed = end - start;
    const bool passed = test->IsPassing();
    AppendResultOrDie(test->GetName(), passed, elapsed.count(), test->outs());
    if (passed) {
      std::cout << "[       OK ] ";
    } else {
      std::cout << "[   FAILED ] ";
    }
    std::cout << entry.suite_name << "." << entry.test_name << "\n";
    failures += (passed) ? 0 : 1;
  }
  return (failures > 0) ? -1 : EXIT_SUCCESS;
}

TestRegistrar::TestRegistrar(const char* suite_name, const char* test_name,
                             std::unique_ptr<TestFactory> factory) {
  TestRegistry::Instance()->RegisterTest(suite_name, test_name,
                                         std::move(factory));
}

}  // namespace internal

// static
Test* Test::Current() { return global_current_test; }

int RunAllTests() { return internal::TestRegistry::Instance()->RunAllTests(); }

}  // namespace testing
