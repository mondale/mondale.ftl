#include <cxxabi.h>
#include <malloc.h>
#include <sanitizer/allocator_interface.h>
#include <sanitizer/common_interface_defs.h>
#include <sanitizer/msan_interface.h>
#include <string.h>

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

#include "base/process.h"
#include "testing/test.h"

namespace testing {
namespace {

void Die() { exit(-1); }

std::string MakeSafeDuration(double secs) {
  return std::format("{:.2f}", secs);
}

class DualAppender final {
 public:
  DualAppender(std::ostream& one, std::ostream& two) : one_(one), two_(two) {}
  template <typename T>
  DualAppender& operator<<(const T& val) {
    one_ << val;
    two_ << val;
    return *this;
  }

 private:
  std::ostream& one_;
  std::ostream& two_;
};

void AppendResultOrDie(const std::string& name, bool pass,
                       double duration_seconds,
                       const std::list<std::string>& fixture_findings,
                       const std::list<std::string>& additional_findings) {
  const auto safe_duration = MakeSafeDuration(duration_seconds);

  // 1. Fetch the environment variable
  const char* result_path = std::getenv("RESULTS_FILE");
  if (!result_path) {
    result_path = "out.txt";
  }

  // 2. Open the destination file for append.
  std::ofstream file(result_path, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file at path [" << result_path << "]"
              << std::endl;
    Die();
  }

  // Mirror findings to std::cout and the file.
  DualAppender da(file, std::cout);

  // 3. Write outcome to the file
  file << "=== RUN   " << name << "\n";
  if (pass) {
    file << "--- ";
    file << "PASS";
    file << ": " << name << " (" << safe_duration << "s)\n";
  } else {
    file << "--- FAIL: " << name << " (" << safe_duration << "s)\n";
    for (const auto& finding : fixture_findings) {
      da << finding << "\n";
    }
    for (const auto& finding : additional_findings) {
      da << finding << "\n";
    }
    file << "FAIL\n";
  }

  file.close();
}

constexpr std::string_view kDisabledPrefix = "DISABLED_";

inline bool IsDisabled(std::string_view test_name) {
  return test_name.starts_with(kDisabledPrefix);
}

void MsanUnpoisonStr(const char* str) {
#ifdef MSAN
  __msan_unpoison_string(str);
#endif
}

std::string Demangle(const char* mangled) {
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
#ifdef MSAN
  if (demangled != nullptr) {
    size_t size = __sanitizer_get_allocated_size(demangled);
    if (size > 0) {
      __msan_unpoison(demangled, size);
    }
  }
#endif
  std::unique_ptr<char, void (*)(void*)> res{demangled, std::free};
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

// Test macros invoked without a running test. This'll make 'em work albeit
// shakily.
class GlobalScopeExpectations : public ::testing::Test {
 public:
  void TestBody() final {}
};
::testing::Test* global_scope_expectation_doohickey =
    new GlobalScopeExpectations();

}  // namespace

std::string Test::GetName() const {
  auto* const p = typeid(*this).name();
  auto ret = StripNamespace(Demangle(p));
  return ret;
}

namespace internal {

// static
TestRegistry* TestRegistry::Instance() {
  static TestRegistry* instance = new TestRegistry();
  return instance;
}

void TestRegistry::RegisterTest(std::string suite_name, std::string test_name,
                                std::unique_ptr<TestFactory> factory) {
  // TODO - Need to move to a model in which the TEST macro decaleras a linked
  // list statically to avoid heap before main() if ever this will work with
  // MSAN.
  tests_.emplace_back(suite_name, test_name, std::move(factory));
}

int TestRegistry::RunAllTests() {
  int passing = 0;
  int failures = 0;
  int disabled = 0;
  for (const auto& entry : tests_) {
    if (IsDisabled(entry.test_name)) {
      std::cout << "[ DISABLED ] " << entry.suite_name << "." << entry.test_name
                << "\n";
      ++disabled;
      continue;
    }

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
    const bool passed =
        test->IsPassing() && global_scope_expectation_doohickey->IsPassing();
    base::FlushLogs();
    const std::string name = test->GetName();
    AppendResultOrDie(name, passed, elapsed.count(), test->outs(),
                      global_scope_expectation_doohickey->outs());
    if (passed) {
      std::cout << "[       OK ] ";
      ++passing;
    } else {
      std::cout << "[   FAILED ] ";
      ++failures;
    }
    std::cout << entry.suite_name << "." << entry.test_name << "\n";
    global_scope_expectation_doohickey->RestorePassing();
  }

  std::cout << "\n[==========] Test Summary\n"
            << "[  PASSED  ] " << passing << " test(s)\n";
  if (failures > 0) {
    std::cout << "[  FAILED  ] " << failures << " test(s)\n";
  }
  if (disabled > 0) {
    std::cout << "[ DISABLED ] " << disabled << " test(s)\n";
  }

  return (failures > 0) ? -1 : EXIT_SUCCESS;
}

TestRegistrar::TestRegistrar(const char* suite_name, const char* test_name,
                             std::unique_ptr<TestFactory> factory) {
  MsanUnpoisonStr(suite_name);
  MsanUnpoisonStr(test_name);
  TestRegistry::Instance()->RegisterTest(suite_name, test_name,
                                         std::move(factory));
}

}  // namespace internal

// static
Test* Test::Current() {
  if (nullptr == global_current_test) {
    return global_scope_expectation_doohickey;
  }
  return global_current_test;
}

int RunAllTests() { return internal::TestRegistry::Instance()->RunAllTests(); }

}  // namespace testing
