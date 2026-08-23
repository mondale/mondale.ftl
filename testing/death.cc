#include <regex>

#include "testing/death.h"

namespace testing {

DeathMatcher DiesWithExitCode(int expected_code) {
  return [expected_code](const DeathTestResult& res, std::string& reason) {
    if (!res.exited_normal) {
      reason = "Process did not exit normally (terminated by signal " +
               std::to_string(res.signal_number) + ")";
      return false;
    }
    if (res.exit_code != expected_code) {
      reason = "Expected exit code " + std::to_string(expected_code) +
               ", got " + std::to_string(res.exit_code);
      return false;
    }
    return true;
  };
}

DeathMatcher StdoutContains(const std::string& pattern) {
  return [pattern](const DeathTestResult& res, std::string& reason) {
    std::regex re(pattern);
    if (!std::regex_search(res.stdout_str, re)) {
      reason = "stdout did not match pattern: \"" + pattern +
               "\"\nActual stdout:\n" + res.stdout_str;
      return false;
    }
    return true;
  };
}

DeathMatcher StderrContains(const std::string& pattern) {
  return [pattern](const DeathTestResult& res, std::string& reason) {
    std::regex re(pattern);
    if (!std::regex_search(res.stderr_str, re)) {
      reason = "stderr did not match pattern: \"" + pattern +
               "\"\nActual stderr:\n" + res.stderr_str;
      return false;
    }
    return true;
  };
}

}  // namespace testing
