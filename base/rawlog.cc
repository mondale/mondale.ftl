#include <ostream>

#include "base/rawlog.h"

namespace base::rawlog {
namespace {

std::ostream* global_rawerror_override = nullptr;
std::ostream* global_rawinfo_override = nullptr;

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

}  // namespace base::rawlog
