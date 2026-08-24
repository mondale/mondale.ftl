#ifndef TESTING_MOCK_H_
#define TESTING_MOCK_H_

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "testing/test.h"

namespace testing {
namespace internal {

// Helper to check if a type T supports operator<<
template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>()
                                             << std::declval<T>())>>
    : std::true_type {};

template <typename T>
void PrintArg(std::ostream& os, const T& val) {
  if constexpr (is_streamable<T>::value) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      os << "\"" << val << "\"";
    } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
      os << "'" << val << "'";
    } else {
      os << val;
    }
  } else {
    os << "<unprintable " << sizeof(T) << "-byte value>";
  }
}

// Format a tuple of arguments into "(arg1, arg2, ...)"
template <typename Tuple, std::size_t... Is>
void PrintArgsTuple(std::ostream& os, const Tuple& tuple,
                    std::index_sequence<Is...>) {
  os << "(";
  std::size_t n = 0;
  ((PrintArg(os, std::get<Is>(tuple)), os << (++n < sizeof...(Is) ? ", " : "")),
   ...);
  os << ")";
}

template <typename... Args>
std::string PrintArgs(const std::tuple<Args...>& tuple) {
  if constexpr (sizeof...(Args) == 0) {
    return "()";
  } else {
    std::ostringstream os;
    PrintArgsTuple(os, tuple, std::index_sequence_for<Args...>{});
    return os.str();
  }
}

// Generic interface for an expectation attached to a mock method
template <typename Ret, typename... Args>
class Expectation {
 public:
  virtual ~Expectation() = default;
  virtual bool Matches(const std::tuple<Args...>& args) const = 0;
  virtual Ret Invoke(Args... args) = 0;
  virtual bool IsSatisfied() const = 0;
  virtual void DescribeTo(std::ostream& os) const = 0;
  virtual const char* file() const = 0;
  virtual int line() const = 0;
  virtual int expected_calls() const = 0;
  virtual int actual_calls() const = 0;
};

template <typename Ret, typename... Args>
class ExpectationImpl : public Expectation<Ret, Args...> {
 public:
  using MatcherTuple = std::tuple<std::function<bool(const Args&)>...>;

  ExpectationImpl(MatcherTuple matchers, const char* file, int line)
      : matchers_(std::move(matchers)), file_(file), line_(line) {}

  bool Matches(const std::tuple<Args...>& args) const override {
    return MatchArgs(args, std::index_sequence_for<Args...>{});
  }

  // Default expectation is exactly 1 call if not overridden
  bool IsSatisfied() const override {
    int expected = (expected_calls_ >= 0) ? expected_calls_ : 1;
    return actual_calls_ == expected;
  }

  ExpectationImpl& WillOnce(std::function<Ret(Args...)> action) {
    actions_.push_back(std::move(action));
    expected_calls_ = static_cast<int>(actions_.size());
    return *this;
  }

  ExpectationImpl& WillRepeatedly(std::function<Ret(Args...)> action) {
    default_action_ = std::move(action);
    // WillRepeatedly allows 0 or more calls unless expected_calls_ was set
    if (expected_calls_ < 0) expected_calls_ = 0;
    return *this;
  }

  Ret Invoke(Args... args) override {
    actual_calls_++;
    if (call_index_ < actions_.size()) {
      return actions_[call_index_++](std::forward<Args>(args)...);
    }
    if (default_action_) {
      return default_action_(std::forward<Args>(args)...);
    }
    return Ret();
  }

  void DescribeTo(std::ostream& os) const override {
    os << file_ << ":" << line_ << ": Expected " << expected_calls_
       << " call(s), actual: " << actual_calls_;
  }

  int expected_calls() const override {
    return (expected_calls_ >= 0) ? expected_calls_ : 1;
  }
  int actual_calls() const override { return actual_calls_; }
  const char* file() const override { return file_; }
  int line() const override { return line_; }

 private:
  template <std::size_t... Is>
  bool MatchArgs(const std::tuple<Args...>& args,
                 std::index_sequence<Is...>) const {
    return (... && std::get<Is>(matchers_)(std::get<Is>(args)));
  }

  MatcherTuple matchers_;
  std::vector<std::function<Ret(Args...)>> actions_;
  std::function<Ret(Args...)> default_action_;
  std::size_t call_index_ = 0;
  int expected_calls_ = -1;  // -1 signifies "unset, default to 1"
  int actual_calls_ = 0;
  const char* file_;
  int line_;
};

// Registry attached to each mock method inside a mock object
template <typename Ret, typename... Args>
class MockSpec {
 public:
  explicit MockSpec(const char* name) : method_name_(name) {}

  ~MockSpec() { VerifyAndClear(); }

  void VerifyAndClear() {
    for (const auto& exp : expectations_) {
      if (!exp->IsSatisfied()) {
        std::ostringstream os;
        os << "Actual function call count doesn't match EXPECT_CALL("
           << method_name_ << ")...\n"
           << "  Expected: to be called " << exp->expected_calls()
           << " time(s)\n"
           << "  Actual: called " << exp->actual_calls() << " time(s)";

        Test::Current()->AddFailure(exp->file(), exp->line(), os.str());
      }
    }
    expectations_.clear();
  }

  ExpectationImpl<Ret, Args...>& AddExpectation(
      typename ExpectationImpl<Ret, Args...>::MatcherTuple matchers,
      const char* file, int line) {
    auto exp = std::make_unique<ExpectationImpl<Ret, Args...>>(
        std::move(matchers), file, line);
    auto* ptr = exp.get();
    expectations_.push_back(std::move(exp));
    return *ptr;
  }

  Ret Invoke(Args... args) {
    auto arg_tuple = std::tie(args...);
    for (auto& exp : expectations_) {
      if (exp->Matches(arg_tuple)) {
        return exp->Invoke(std::forward<Args>(args)...);
      }
    }

    // Unmatched call failure with formatted argument list
    std::ostringstream os;
    os << "Unexpected call to " << method_name_ << PrintArgs(arg_tuple);

    if (!expectations_.empty()) {
      os << "\n  No matching expectation found.";
    }

    Test::Current()->AddFailure(__FILE__, __LINE__, os.str());

    return Ret();
  }

 private:
  std::string method_name_;
  // The vector holding all registered expectations for this method
  std::vector<std::unique_ptr<Expectation<Ret, Args...>>> expectations_;
};

template <typename T>
std::function<bool(const T&)> MakeMatcher(T expected_value) {
  return [expected = std::move(expected_value)](const T& actual) {
    return actual == expected;
  };
}

template <typename T>
std::function<bool(const T&)> MakeMatcher(
    std::function<bool(const T&)> matcher_fn) {
  return matcher_fn;
}

// Helper to convert matcher or literal to std::function<bool(const T&)>
template <typename TargetType, typename MatcherOrValue>
auto ImplicitMatcher(MatcherOrValue&& mv) {
  if constexpr (std::is_invocable_r_v<bool, MatcherOrValue,
                                      const TargetType&>) {
    return std::function<bool(const TargetType&)>(
        std::forward<MatcherOrValue>(mv));
  } else {
    return std::function<bool(const TargetType&)>(
        [expected = std::forward<MatcherOrValue>(mv)](
            const TargetType& actual) { return actual == expected; });
  }
}

template <typename ExpectationType>
struct ExpectationProxy {
  ExpectationType& expectation;

  template <typename... Args>
  ExpectationType& operator()(Args&&...) {
    return expectation;
  }
};

}  // namespace internal

// --- Arity 0 ---
#define MOCK_METHOD0(Ret, Name, Specifiers)                             \
  mutable ::testing::internal::MockSpec<Ret> mock_##Name{#Name};        \
  Ret Name() Specifiers { return mock_##Name.Invoke(); }                \
  auto _expect_##Name() {                                               \
    return [this](const char* file, int line) -> auto& {                \
      return mock_##Name.AddExpectation(std::make_tuple(), file, line); \
    };                                                                  \
  }

// --- Arity 1 ---
#define MOCK_METHOD1(Ret, Name, Type1, ...)                               \
  mutable ::testing::internal::MockSpec<Ret, Type1> mock_##Name{#Name};   \
  Ret Name(Type1 a1) __VA_ARGS__ { return mock_##Name.Invoke(a1); }       \
  template <typename M1>                                                  \
  auto _expect_##Name(M1&& m1) {                                          \
    return [this, m1 = std::forward<M1>(m1)](const char* file,            \
                                             int line) -> auto& {         \
      auto matchers = std::make_tuple(                                    \
          ::testing::internal::ImplicitMatcher<Type1>(std::move(m1)));    \
      return mock_##Name.AddExpectation(std::move(matchers), file, line); \
    };                                                                    \
  }

// --- Arity 2 ---
#define MOCK_METHOD2(Ret, Name, Type1, Type2, ...)                             \
  mutable ::testing::internal::MockSpec<Ret, Type1, Type2> mock_##Name{#Name}; \
  Ret Name(Type1 a1, Type2 a2) __VA_ARGS__ {                                   \
    return mock_##Name.Invoke(a1, a2);                                         \
  }                                                                            \
  template <typename M1, typename M2>                                          \
  auto _expect_##Name(M1&& m1, M2&& m2) {                                      \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2)](       \
               const char* file, int line) -> auto& {                          \
      auto matchers = std::make_tuple(                                         \
          ::testing::internal::ImplicitMatcher<Type1>(std::move(m1)),          \
          ::testing::internal::ImplicitMatcher<Type2>(std::move(m2)));         \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);      \
    };                                                                         \
  }

// --- Arity 3 ---
#define MOCK_METHOD3(Ret, Name, T1, T2, T3, ...)                              \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3> mock_##Name{#Name};  \
  Ret Name(T1 a1, T2 a2, T3 a3) __VA_ARGS__ {                                 \
    return mock_##Name.Invoke(a1, a2, a3);                                    \
  }                                                                           \
  template <typename M1, typename M2, typename M3>                            \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3) {                            \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 4 ---
#define MOCK_METHOD4(Ret, Name, T1, T2, T3, T4, ...)                          \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4> mock_##Name{     \
      #Name};                                                                 \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4) __VA_ARGS__ {                          \
    return mock_##Name.Invoke(a1, a2, a3, a4);                                \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4>               \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4) {                   \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3),                                        \
            m4 = std::forward<M4>(m4)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 5 ---
#define MOCK_METHOD5(Ret, Name, T1, T2, T3, T4, T5, ...)                      \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5> mock_##Name{ \
      #Name};                                                                 \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) __VA_ARGS__ {                   \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5);                            \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4, typename M5>  \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5) {          \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),             \
            m5 = std::forward<M5>(m5)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),            \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 6 ---
#define MOCK_METHOD6(Ret, Name, T1, T2, T3, T4, T5, T6, ...)                  \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5, T6>          \
      mock_##Name{#Name};                                                     \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) __VA_ARGS__ {            \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5, a6);                        \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4, typename M5,  \
            typename M6>                                                      \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5, M6&& m6) { \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),             \
            m5 = std::forward<M5>(m5),                                        \
            m6 = std::forward<M6>(m6)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),            \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)),            \
          ::testing::internal::ImplicitMatcher<T6>(std::move(m6)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 7 ---
#define MOCK_METHOD7(Ret, Name, T1, T2, T3, T4, T5, T6, T7, ...)              \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5, T6, T7>      \
      mock_##Name{#Name};                                                     \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) __VA_ARGS__ {     \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5, a6, a7);                    \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4, typename M5,  \
            typename M6, typename M7>                                         \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5, M6&& m6,   \
                      M7&& m7) {                                              \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),             \
            m5 = std::forward<M5>(m5), m6 = std::forward<M6>(m6),             \
            m7 = std::forward<M7>(m7)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),            \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)),            \
          ::testing::internal::ImplicitMatcher<T6>(std::move(m6)),            \
          ::testing::internal::ImplicitMatcher<T7>(std::move(m7)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 8 ---
#define MOCK_METHOD8(Ret, Name, T1, T2, T3, T4, T5, T6, T7, T8, ...)          \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5, T6, T7, T8>  \
      mock_##Name{#Name};                                                     \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8)            \
      __VA_ARGS__ {                                                           \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5, a6, a7, a8);                \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4, typename M5,  \
            typename M6, typename M7, typename M8>                            \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5, M6&& m6,   \
                      M7&& m7, M8&& m8) {                                     \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),             \
            m5 = std::forward<M5>(m5), m6 = std::forward<M6>(m6),             \
            m7 = std::forward<M7>(m7),                                        \
            m8 = std::forward<M8>(m8)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),            \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)),            \
          ::testing::internal::ImplicitMatcher<T6>(std::move(m6)),            \
          ::testing::internal::ImplicitMatcher<T7>(std::move(m7)),            \
          ::testing::internal::ImplicitMatcher<T8>(std::move(m8)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 9 ---
#define MOCK_METHOD9(Ret, Name, T1, T2, T3, T4, T5, T6, T7, T8, T9, ...)      \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5, T6, T7, T8,  \
                                        T9>                                   \
      mock_##Name{#Name};                                                     \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9)     \
      __VA_ARGS__ {                                                           \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5, a6, a7, a8, a9);            \
  }                                                                           \
  template <typename M1, typename M2, typename M3, typename M4, typename M5,  \
            typename M6, typename M7, typename M8, typename M9>               \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5, M6&& m6,   \
                      M7&& m7, M8&& m8, M9&& m9) {                            \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),       \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),             \
            m5 = std::forward<M5>(m5), m6 = std::forward<M6>(m6),             \
            m7 = std::forward<M7>(m7), m8 = std::forward<M8>(m8),             \
            m9 = std::forward<M9>(m9)](const char* file, int line) -> auto& { \
      auto matchers = std::make_tuple(                                        \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),            \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),            \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),            \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),            \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)),            \
          ::testing::internal::ImplicitMatcher<T6>(std::move(m6)),            \
          ::testing::internal::ImplicitMatcher<T7>(std::move(m7)),            \
          ::testing::internal::ImplicitMatcher<T8>(std::move(m8)),            \
          ::testing::internal::ImplicitMatcher<T9>(std::move(m9)));           \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);     \
    };                                                                        \
  }

// --- Arity 10 ---
#define MOCK_METHOD10(Ret, Name, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, ...) \
  mutable ::testing::internal::MockSpec<Ret, T1, T2, T3, T4, T5, T6, T7, T8,   \
                                        T9, T10>                               \
      mock_##Name{#Name};                                                      \
  Ret Name(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9,      \
           T10 a10) __VA_ARGS__ {                                              \
    return mock_##Name.Invoke(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);        \
  }                                                                            \
  template <typename M1, typename M2, typename M3, typename M4, typename M5,   \
            typename M6, typename M7, typename M8, typename M9, typename M10>  \
  auto _expect_##Name(M1&& m1, M2&& m2, M3&& m3, M4&& m4, M5&& m5, M6&& m6,    \
                      M7&& m7, M8&& m8, M9&& m9, M10&& m10) {                  \
    return [this, m1 = std::forward<M1>(m1), m2 = std::forward<M2>(m2),        \
            m3 = std::forward<M3>(m3), m4 = std::forward<M4>(m4),              \
            m5 = std::forward<M5>(m5), m6 = std::forward<M6>(m6),              \
            m7 = std::forward<M7>(m7), m8 = std::forward<M8>(m8),              \
            m9 = std::forward<M9>(m9), m10 = std::forward<M10>(m10)](          \
               const char* file, int line) -> auto& {                          \
      auto matchers = std::make_tuple(                                         \
          ::testing::internal::ImplicitMatcher<T1>(std::move(m1)),             \
          ::testing::internal::ImplicitMatcher<T2>(std::move(m2)),             \
          ::testing::internal::ImplicitMatcher<T3>(std::move(m3)),             \
          ::testing::internal::ImplicitMatcher<T4>(std::move(m4)),             \
          ::testing::internal::ImplicitMatcher<T5>(std::move(m5)),             \
          ::testing::internal::ImplicitMatcher<T6>(std::move(m6)),             \
          ::testing::internal::ImplicitMatcher<T7>(std::move(m7)),             \
          ::testing::internal::ImplicitMatcher<T8>(std::move(m8)),             \
          ::testing::internal::ImplicitMatcher<T9>(std::move(m9)),             \
          ::testing::internal::ImplicitMatcher<T10>(std::move(m10)));          \
      return mock_##Name.AddExpectation(std::move(matchers), file, line);      \
    };                                                                         \
  }

#define EXPECT_CALL(obj, call) ((obj)._expect_##call)(__FILE__, __LINE__)

// Helper actions
template <typename T>
auto Return(T val) {
  return [val = std::move(val)](auto&&...) { return val; };
}

// Inline matchers.
// Greater than or equal to
template <typename V>
auto Ge(V val) {
  return [val = std::move(val)](const auto& actual) { return actual >= val; };
}

// Less than
template <typename V>
auto Lt(V val) {
  return [val = std::move(val)](const auto& actual) { return actual < val; };
}

// Greater than
template <typename V>
auto Gt(V val) {
  return [val = std::move(val)](const auto& actual) { return actual > val; };
}

// Less than or equal to
template <typename V>
auto Le(V val) {
  return [val = std::move(val)](const auto& actual) { return actual <= val; };
}

// Any value (wildcard matcher)
inline auto _() {
  return [](const auto&) { return true; };
}

}  // namespace testing

#endif  // #ifndef TESTING_MOCK_H_
