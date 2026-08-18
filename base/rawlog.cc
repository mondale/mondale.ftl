#include <signal.h>

#include <iostream>
#include <ostream>
#include <streambuf>

#include "base/rawlog.h"

namespace base::rawlog {
namespace {

std::ostream* global_rawerror_override = nullptr;
std::ostream* global_rawinfo_override = nullptr;

class NullStreambuf final : public std::streambuf {
 private:
  // Handle single-character writes (e.g., std::endl, individual chars)
  int_type overflow(int_type c) override {
    return traits_type::not_eof(c);  // Return non-EOF to report success
  }

  // Handle bulk writes (e.g., string literals, std::string_view, std::string)
  std::streamsize xsputn(const char* /*s*/, std::streamsize n) override {
    return n;  // Pretend all n characters were written
  }
};

class NullOstream : public std::ostream {
 public:
  NullOstream() : std::ostream(&buf_) {}

 private:
  NullStreambuf buf_;
};

NullOstream global_null_ostream;

}  // namespace

void TESTONLY_SetInfoStream(std::ostream* out) {
  global_rawinfo_override = out;
}

std::ostream* GetInfoStream() {
  if (nullptr == global_rawinfo_override) return &std::cout;
  return global_rawinfo_override;
}

void TESTONLY_SetErrorStream(std::ostream* out) {
  global_rawerror_override = out;
}

std::ostream* GetErrorStream() {
  if (nullptr == global_rawerror_override) return &std::cerr;
  return global_rawerror_override;
}

CheckHelper::CheckHelper(const char* file, int line, bool condition,
                         const char* expression)
    : condition_(condition),
      stream_(condition ? global_null_ostream : *GetErrorStream()) {
  if (condition_) return;
  stream() << "CHECK failure [";
  stream() << file << ":" << line << "](";
  stream() << expression << ")\n";
}

CheckHelper::~CheckHelper() {
  if (condition_) return;
  stream() << "\n";
  raise(SIGABRT);
}

}  // namespace base::rawlog
