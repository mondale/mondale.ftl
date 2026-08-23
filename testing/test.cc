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

std::list<std::unique_ptr<BasicTest>>* global_all_tests = nullptr;

}  // namespace

std::string BasicTest::GetName() const {
  return StripNamespace(Demangle(typeid(*this).name()));
}

void BasicTest::AddFailure(const char* file, int line, std::string message) {
  passing_ = false;
  std::stringstream ss;
  ss << file << "[" << line << "]: " << message;
  outs_.push_back(ss.str());
}

bool RegisterTest(std::unique_ptr<BasicTest> test) {
  if (nullptr == global_all_tests) {
    global_all_tests = new std::list<std::unique_ptr<BasicTest>>();
  }
  global_all_tests->push_back(std::move(test));
  return true;
}

int RunAllTests() {
  int failures = 0;
  if (nullptr == global_all_tests) return EXIT_SUCCESS;
  for (auto& test : *global_all_tests) {
    const auto start = std::chrono::steady_clock::now();
    test->Run();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    const bool passed = test->IsPassing();
    AppendResultOrDie(test->GetName(), passed, elapsed.count(), test->outs());
    failures += (passed) ? 0 : 1;
  }
  return (failures > 0) ? -1 : EXIT_SUCCESS;
}

}  // namespace testing
