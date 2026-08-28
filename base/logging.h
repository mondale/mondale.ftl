#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <signal.h>

#include <sstream>
#include <string>

#include "base/cpu.h"
#include "base/log_writer_thread.h"
#include "base/logging_internal.h"
#include "base/source_location.h"
#include "base/thread.h"
#include "base/time.h"

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

struct NilModifier {};

inline bool EvaluateModifier(NilModifier) { return true; }

namespace internal {

std::string GetLogPath();

}  // namespace internal
}  // namespace base

#define _LOG_SELECT_NAME(_1, _2, NAME, ...) NAME
#define _LOG_CHOOSER(...) _LOG_SELECT_NAME(__VA_ARGS__, _LOG_2, _LOG_1)

#define _LOG_1(sev) \
  ::base::LogMessageProxy(sev, ::base::SourceLocation::Current())

#define _LOG_2(sev, mod)             \
  if (::base::EvaluateModifier(mod)) \
  ::base::LogMessageProxy(sev, ::base::SourceLocation::Current())

#define Log(...) _LOG_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define VLOG(n) \
  if (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())

#ifdef NDEBUG
#define DVLOG(n) \
  while (false) ::base::LogMessageProxy(INFO, ::base::SourceLocation::Current())
#else
#define DVLOG(n) VLOG(n)
#endif

#define CHECK(cond)                                                 \
  if (auto v = (cond); !v)                                          \
  ::base::LogMessageProxy(FATAL, ::base::SourceLocation::Current()) \
      << "Check failed: " << #cond << " "

#define CHECK_OP(name, op, v1, v2)                                           \
  if (auto val1 = (v1), val2 = (v2); !(val1 op val2))                        \
  ::base::LogMessageProxy(FATAL, ::base::SourceLocation::Current())          \
      << "Check failed: " << #v1 << " " << #op << " " << #v2 << " (" << val1 \
      << " vs " << val2 << ") "

#define CHECK_EQ(v1, v2) CHECK_OP(_EQ, ==, v1, v2)
#define CHECK_NE(v1, v2) CHECK_OP(_NE, !=, v1, v2)
#define CHECK_LE(v1, v2) CHECK_OP(_LE, <=, v1, v2)
#define CHECK_LT(v1, v2) CHECK_OP(_LT, <, v1, v2)
#define CHECK_GE(v1, v2) CHECK_OP(_GE, >=, v1, v2)
#define CHECK_GT(v1, v2) CHECK_OP(_GT, >, v1, v2)

#define CHECK_OK(result)                                            \
  if (auto r = (result); !IsOk(r))                                  \
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

#define DCHECK_OP(name, op, v1, v2)                                          \
  if (auto val1 = (v1), val2 = (v2); !(val1 op val2))                        \
  ::base::LogMessageProxy(DFATAL, ::base::SourceLocation::Current())         \
      << "Check failed: " << #v1 << " " << #op << " " << #v2 << " (" << val1 \
      << " vs " << val2 << ") "

#define DCHECK_EQ(v1, v2) DCHECK_OP(_EQ, ==, v1, v2)
#define DCHECK_NE(v1, v2) DCHECK_OP(_NE, !=, v1, v2)
#define DCHECK_LE(v1, v2) DCHECK_OP(_LE, <=, v1, v2)
#define DCHECK_LT(v1, v2) DCHECK_OP(_LT, <, v1, v2)
#define DCHECK_GE(v1, v2) DCHECK_OP(_GE, >=, v1, v2)
#define DCHECK_GT(v1, v2) DCHECK_OP(_GT, >, v1, v2)
#endif

#endif  // BASE_LOGGING_H_
