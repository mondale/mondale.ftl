#ifndef TESTING_DEATH_H_
#define TESTING_DEATH_H_

#include <functional>
#include <string>

namespace testing {

struct DeathTestResult {
  bool exited_normal = false;
  int exit_code = -1;
  bool killed_by_signal = false;
  int signal_number = -1;
  bool timed_out = false;
  std::string stdout_str;
  std::string stderr_str;

  bool Died() const {
    return (exited_normal && exit_code != 0) || killed_by_signal;
  }
};

using DeathMatcher = std::function<bool(
    const DeathTestResult&, std::string& /* failure_reason(output) */)>;

// Matchers!
DeathMatcher DiesWithExitCode(int expected_code);
DeathMatcher StdoutContains(const std::string& pattern);
DeathMatcher StderrContains(const std::string& pattern);
DeathMatcher TimedOut();

// Combine matchers using variadic templates or operator&&
template <typename M1, typename M2>
DeathMatcher operator&&(M1&& m1, M2&& m2) {
  return [m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2)](
             const DeathTestResult& res, std::string& reason) {
    return m1(res, reason) && m2(res, reason);
  };
}

}  // namespace testing

#endif  // #ifndef TESTING_DEATH_H_
