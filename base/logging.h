#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <signal.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "base/cpu.h"
#include "base/log_writer_thread.h"
#include "base/logging_internal.h"
#include "base/source_location.h"
#include "base/thread.h"
#include "base/time.h"

// TODO - flush logs at end of a test case.

inline constexpr base::internal::LogSeverity INFO =
    base::internal::LogSeverity::kInfo;
inline constexpr base::internal::LogSeverity WARNING =
    base::internal::LogSeverity::kWarning;
inline constexpr base::internal::LogSeverity ERROR =
    base::internal::LogSeverity::kError;
inline constexpr base::internal::LogSeverity FATAL =
    base::internal::LogSeverity::kFatal;

#ifdef NDEBUG
inline constexpr base::internal::LogSeverity DFATAL =
    base::internal::LogSeverity::kError;
#else
inline constexpr base::internal::LogSeverity DFATAL =
    base::internal::LogSeverity::kFatal;
#endif

namespace base::internal {

bool VlogIsOnSlow(int level, base::SourceLocation loc);

extern std::atomic<int> g_logging_verbosity;

inline bool VlogIsOn(int level, base::SourceLocation loc) {
  const auto enabled_verbosity =
      g_logging_verbosity.load(std::memory_order_relaxed);
  if (enabled_verbosity < level) {
    return false;
  }
  return VlogIsOnSlow(level, loc);
}

inline void SubmitLogEntry(LogSeverity severity, base::SourceLocation loc,
                           std::string message) {
  LogEntry entry;
  entry.severity = severity;
  entry.timestamp = base::WallTime::Now();
  entry.tid = base::GetCachedTid();
  entry.file = loc.file();
  entry.line = loc.line();
  entry.message = std::move(message);

  const int cpu = base::CurrentCpu();
  if (FATAL != severity) {
    LogWriterThread::Instance()->QueueForCpu(cpu)->Push(std::move(entry));
    return;
  }

  // Fatal path.
  std::cerr << entry.ToString() << std::endl;
  raise(SIGABRT);
}

struct OpEq {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_equal(v1, v2);
    } else {
      return v1 == v2;
    }
  }
};

struct OpNe {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_not_equal(v1, v2);
    } else {
      return v1 != v2;
    }
  }
};

struct OpLt {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_less(v1, v2);
    } else {
      return v1 < v2;
    }
  }
};

struct OpLe {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_less_equal(v1, v2);
    } else {
      return v1 <= v2;
    }
  }
};

struct OpGt {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_greater(v1, v2);
    } else {
      return v1 > v2;
    }
  }
};

struct OpGe {
  template <typename T1, typename T2>
  bool operator()(const T1& v1, const T2& v2) const {
    if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
      return std::cmp_greater_equal(v1, v2);
    } else {
      return v1 >= v2;
    }
  }
};

}  // namespace base::internal

namespace base {

class LogMessageProxy final {
 public:
  LogMessageProxy(internal::LogSeverity sev, base::SourceLocation loc)
      : sev_(sev), loc_(loc) {}

  ~LogMessageProxy() {
    internal::SubmitLogEntry(sev_, loc_, std::move(stream_).str());
  }

  template <typename T>
  LogMessageProxy& operator<<(const T& val) {
    stream_ << val;
    return *this;
  }

  LogMessageProxy& operator<<(std::ostream& (*fn)(std::ostream&)) {
    fn(stream_);
    return *this;
  }

  LogMessageProxy& operator<<(std::ios_base& (*fn)(std::ios_base&)) {
    fn(stream_);
    return *this;
  }

 private:
  internal::LogSeverity sev_;
  base::SourceLocation loc_;
  std::ostringstream stream_;
};

struct NilModifier {
  bool Evaluate() const { return true; }
};

struct FirstModifier {
  int lim;
  std::atomic<int>* cnt;

  bool Evaluate() const {
    int val = cnt->load(std::memory_order_relaxed);
    if (val < lim) {
      cnt->store(val + 1, std::memory_order_relaxed);
      return true;
    }
    return false;
  }
};

struct EveryModifier {
  int intv;
  std::atomic<int>* cnt;

  bool Evaluate() const {
    int val = cnt->fetch_add(1, std::memory_order_relaxed);
    return (val % intv) == 0;
  }
};

struct IfModifier {
  bool cnd;

  bool Evaluate() const { return cnd; }
};

template <typename T>
inline bool EvaluateModifier(const T& mod) {
  return mod.Evaluate();
}

void SetVlogLevel(int level);
void SetVmodules(std::string_view vmodules);

}  // namespace base

#define FIRST_IMPL(n)                        \
  ([]() {                                    \
    static std::atomic<int> s_cnt{0};        \
    return ::base::FirstModifier{n, &s_cnt}; \
  }())

#define EVERY_IMPL(n)                        \
  ([]() {                                    \
    static std::atomic<int> s_cnt{0};        \
    return ::base::EveryModifier{n, &s_cnt}; \
  }())

#define IF_IMPL(c) \
  ::base::IfModifier { static_cast<bool>(c) }

#define First(n) FIRST_IMPL(n)
#define Every(n) EVERY_IMPL(n)
#define If(c) IF_IMPL(c)

#define _LOG_SELECT_NAME(_1, _2, NAME, ...) NAME
#define _LOG_CHOOSER(...) _LOG_SELECT_NAME(__VA_ARGS__, _LOG_2, _LOG_1)

#define _LOG_1(sev) \
  ::base::LogMessageProxy(sev, ::base::SourceLocation::Current())

#define _LOG_2(sev, mod)             \
  if (::base::EvaluateModifier(mod)) \
  ::base::LogMessageProxy(sev, ::base::SourceLocation::Current())

#define Log(...) _LOG_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define VLOG(n)                                                         \
  if (::base::internal::VlogIsOn(n, ::base::SourceLocation::Current())) \
  ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())

#ifdef NDEBUG
#define DVLOG(n) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#else
#define DVLOG(n) VLOG(n)
#endif

#ifdef NDEBUG
#define DVLOG(n) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#else
#define DVLOG(n) VLOG(n)
#endif

#define CHECK_OP(name, op_struct, v1, v2)                                      \
  if (auto val1 = (v1); true)                                                  \
    if (auto val2 = (v2); !::base::internal::op_struct{}(val1, val2))          \
  ::base::LogMessageProxy(FATAL, ::base::SourceLocation::Current())            \
      << "Check failed: " << #v1 << " " << #name << " " << #v2 << " (" << val1 \
      << " vs " << val2 << ") "

#define CHECK_EQ(v1, v2) CHECK_OP(==, OpEq, v1, v2)
#define CHECK_NE(v1, v2) CHECK_OP(!=, OpNe, v1, v2)
#define CHECK_LE(v1, v2) CHECK_OP(<=, OpLe, v1, v2)
#define CHECK_LT(v1, v2) CHECK_OP(<, OpLt, v1, v2)
#define CHECK_GE(v1, v2) CHECK_OP(>=, OpGe, v1, v2)
#define CHECK_GT(v1, v2) CHECK_OP(>, OpGt, v1, v2)

#define CHECK(cond)                                                 \
  if (auto v = (cond); !v)                                          \
  ::base::LogMessageProxy(FATAL, ::base::SourceLocation::Current()) \
      << "Check failed: " << #cond << " "

#define CHECK_OK(result)                                            \
  if (const auto& r = (result); !IsOk(r))                           \
  ::base::LogMessageProxy(FATAL, ::base::SourceLocation::Current()) \
      << "Check failed: " << #result << " is not OK: " << r

#ifdef NDEBUG
#define DCHECK(cond) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_EQ(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_NE(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_LE(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_LT(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_GE(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#define DCHECK_GT(v1, v2) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#else
#define DCHECK(cond)                                                 \
  if (auto v = (cond); !v)                                           \
  ::base::LogMessageProxy(DFATAL, ::base::SourceLocation::Current()) \
      << "Check failed: " << #cond << " "

#define DCHECK_OP(name, op_struct, v1, v2)                                     \
  if (auto val1 = (v1); true)                                                  \
    if (auto val2 = (v2); !::base::internal::op_struct{}(val1, val2))          \
  ::base::LogMessageProxy(DFATAL, ::base::SourceLocation::Current())           \
      << "Check failed: " << #v1 << " " << #name << " " << #v2 << " (" << val1 \
      << " vs " << val2 << ") "

#define DCHECK_EQ(v1, v2) DCHECK_OP(==, OpEq, v1, v2)
#define DCHECK_NE(v1, v2) DCHECK_OP(!=, OpNe, v1, v2)
#define DCHECK_LE(v1, v2) DCHECK_OP(<=, OpLe, v1, v2)
#define DCHECK_LT(v1, v2) DCHECK_OP(<, OpLt, v1, v2)
#define DCHECK_GE(v1, v2) DCHECK_OP(>=, OpGe, v1, v2)
#define DCHECK_GT(v1, v2) DCHECK_OP(>, OpGt, v1, v2)

#endif

#endif  // BASE_LOGGING_H_
