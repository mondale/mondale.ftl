#include <list>
#include <set>

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

Result Error(int line, const std::string& msg) {
  return Result(Code::kStreamFatal, Format("Line {}: {}", line, msg));
}

Result Error(const std::string& srcloc, const std::string& msg) {
  return Result(Code::kStreamFatal, Format("{}: {}", srcloc, msg));
}

bool IsPrimitiveType(std::string_view t) {
  if (t == "bool") return true;
  if (t == "f32") return true;
  if (t == "f64") return true;
  if (t == "i16") return true;
  if (t == "i32") return true;
  if (t == "i64") return true;
  if (t == "i8") return true;
  if (t == "string") return true;
  if (t == "u16") return true;
  if (t == "u32") return true;
  if (t == "u64") return true;
  if (t == "u8") return true;
  return false;
}

bool IsVectorType(std::string_view t) { return 0 == (t.rfind("vector<", 0)); }

std::string VectorToInner(std::string_view t) {
  if (t.length() < 7) return "";
  if (t.rfind("vector<", 0) == 0) {
    return std::string(t.substr(7, t.size() - 8));
  }
  return "";
}

}  // namespace

Result VerifyTypeSoundness(const CapsuleFile& cf) {
  // Types are sound when all types named are either...
  //  * A primitive type.
  //  * A capsule type also defined in the file.
  //  * A vector of primivives or capsules.
  std::list<Field> sus;
  std::set<std::string> cnames;
  Result ret = Result::Ok();

  // Build index of capsule names in the file.
  for (const auto& c : cf.capsules) {
    cnames.insert(c.name);
  }

  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      // Primitives ok.
      if (IsPrimitiveType(f.type)) continue;

      // Vectors ok when their inner types are ok.
      if (IsVectorType(f.type)) {
        const auto i = VectorToInner(f.type);
        if (IsPrimitiveType(i)) continue;
        if (cnames.find(i) != cnames.end()) continue;
        sus.push_back(f);
        continue;
      }

      // Everything else has to be a capsule.
      if (cnames.find(f.type) != cnames.end()) continue;
      sus.push_back(f);
    }
  }

  for (const auto& f : sus) {
    Accumulate(
        &ret,
        Error(f.srcloc,
              Format("Unrecognized primitive, vector, or capsule type [{}].",
                     f.type)));
  }
  return ret;
}

Result VerifyAtLeastOneCapsule(const CapsuleFile& cf) {
  if (cf.capsules.empty()) {
    return Error(0, Format("No capsules defined."));
  }
  return Result::Ok();
}

Result Verify(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  Accumulate(&ret, VerifyAtLeastOneCapsule(cf));
  Accumulate(&ret, VerifyTypeSoundness(cf));
  return ret;
}

}  // namespace capsule
