#include "base/rawlog.h"

namespace base::rawlog {
namespace {

std::ostream* global_rawinfo_override = nullptr;

}  // namespace

void TESTONLY_SetInfoStream(std::ostream* out) {
  // RAWCHECK(nullptr == global_rawinfo_override);
  global_rawinfo_override = out;
}

std::ostream* GetInfoStream() {
  if (nullptr == global_rawinfo_override) return &std::cout;
  return global_rawinfo_override;
}

}  // namespace base::rawlog
