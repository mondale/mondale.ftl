#ifndef BASE_RAW_LOG_H_
#define BASE_RAW_LOG_H_

#include <iostream>
#include <ostream>

namespace base::rawlog {

void TESTONLY_SetInfoStream(std::ostream* out);
std::ostream* GetInfoStream();
void TESTONLY_SetErrorStream(std::ostream* out);
std::ostream* GetErrorStream();
void TESTONLY_SetWarningStream(std::ostream* out);
std::ostream* GetWarningStream();

class RawLogger final {
 public:
  RawLogger(bool fatal, const char* prefix, const char* file, int line,
            std::ostream& stream)
      : fatal_(fatal), stream_(stream) {
    stream_ << prefix << file << ":" << line << "] ";
  }
  ~RawLogger() {
    stream_ << std::endl << std::flush;
    if (fatal_) exit(1);
  }
  std::ostream& stream() { return stream_; }

 private:
  const bool fatal_;
  std::ostream& stream_;
};

class CheckHelper final {
 public:
  CheckHelper(const char* file, int line, bool condition,
              const char* expression);
  ~CheckHelper();

  std::ostream& stream() { return stream_; }

 private:
  const bool condition_;
  std::ostream& stream_;
};

}  // namespace base::rawlog

#define RAW_INFO                                                              \
  (::base::rawlog::RawLogger(false, "I ", __builtin_FILE(), __builtin_LINE(), \
                             *::base::rawlog::GetInfoStream())                \
       .stream())
#define RAW_ERROR                                                             \
  (::base::rawlog::RawLogger(false, "E ", __builtin_FILE(), __builtin_LINE(), \
                             *::base::rawlog::GetErrorStream())               \
       .stream())
#define RAW_WARNING                                                           \
  (::base::rawlog::RawLogger(false, "W ", __builtin_FILE(), __builtin_LINE(), \
                             *::base::rawlog::GetWarningStream())             \
       .stream())
#define RAW_FATAL                                                            \
  (::base::rawlog::RawLogger(true, "F ", __builtin_FILE(), __builtin_LINE(), \
                             *::base::rawlog::GetErrorStream())              \
       .stream())

#define RAW_CHECK(b)                                                      \
  (::base::rawlog::CheckHelper(__builtin_FILE(), __builtin_LINE(), b, #b) \
       .stream())

#ifdef DEBUG
#define RAW_DCHECK(b)                                                     \
  (::base::rawlog::CheckHelper(__builtin_FILE(), __builtin_LINE(), b, #b) \
       .stream())
#else
#define RAW_DCHECK(b)
#endif

#endif  // #ifndef BASE_RAW_LOG_H_
