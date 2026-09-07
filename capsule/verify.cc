#include "capsule/verify.h"

using strings::Format;

namespace capsule {
namespace {

void Accumulate(Result* acc, const Result& r) {
  if (r.IsOk()) return;
  std::string full(acc->message());
  full += "\n";
  full += r.message();
  *acc = Result(r.code(), full);
}

Result Error(int line, const std::string msg) {
  return Result(Code::kStreamFatal, Format("Line {}: {}", line, msg));
}

}  // namespace

Result VerifyAtLeastOneCapsule(const CapsuleFile& cf) {
  if (cf.capsules.empty()) {
    return Error(0, Format("No capsules defined."));
  }
  return Result::Ok();
}

Result Verify(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  Accumulate(&ret, VerifyAtLeastOneCapsule(cf));
  return ret;
}

}  // namespace capsule
