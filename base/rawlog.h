#ifndef BASE_RAW_LOG_H_
#define BASE_RAW_LOG_H_

#include <iostream>
#include <ostream>

namespace base::rawlog {

void TESTONLY_SetInfoStream(std::ostream* out);
std::ostream* GetInfoStream();
void TESTONLY_SetErrorStream(std::ostream* out);
std::ostream* GetErrorStream();

class RawLogger final {
 public:
  RawLogger(bool fatal, const char* prefix, std::ostream& stream)
      : fatal_(fatal), stream_(stream) {
    stream_ << prefix;
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

}  // namespace base::rawlog

#define RAW_INFO                                                             \
  (::base::rawlog::RawLogger(false, "I] ", *::base::rawlog::GetInfoStream()) \
       .stream())
#define RAW_ERROR                                                             \
  (::base::rawlog::RawLogger(false, "E] ", *::base::rawlog::GetErrorStream()) \
       .stream())
#define RAW_FATAL                                                            \
  (::base::rawlog::RawLogger(true, "F] ", *::base::rawlog::GetErrorStream()) \
       .stream())

#endif  // #ifndef BASE_RAW_LOG_H_
