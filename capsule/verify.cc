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

std::list<std::string> AllPrimitiveTypes() {
  std::list<std::string> ret;
  ret.push_back("bool");
  ret.push_back("f32");
  ret.push_back("f64");
  ret.push_back("i16");
  ret.push_back("i32");
  ret.push_back("i64");
  ret.push_back("i8");
  ret.push_back("string");
  ret.push_back("u16");
  ret.push_back("u32");
  ret.push_back("u64");
  ret.push_back("u8");
  return ret;
}

bool IsPrimitiveType(std::string_view t) {
  for (const auto& x : AllPrimitiveTypes()) {
    if (t == x) return true;
  }
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

Result VerifyNamesDistinctFromTypes(const CapsuleFile& cf) {
  Result ret = Result::Ok();

  std::set<std::string> capsule_types;
  for (const auto& c : cf.capsules) {
    capsule_types.insert(c.name);

    // While we're here, complain about poorly named capsules.
    if (IsPrimitiveType(c.name)) {
      Accumulate(
          &ret,
          Error(
              c.srcloc,
              Format("Capsules may not use a primitive typename as a name [{}]",
                     c.name)));
    }
  }

  // Field names may be neither primitive values nor capsule types.
  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      if (IsPrimitiveType(f.name)) {
        Accumulate(
            &ret,
            Error(
                f.srcloc,
                Format("Fields may not use a primitive typename as a name [{}]",
                       f.name)));
      }
      if (capsule_types.find(f.name) != capsule_types.end()) {
        Accumulate(&ret,
                   Error(f.srcloc, Format("Fields may not share a name with a "
                                          "capsule in the same file [{}]",
                                          f.name)));
      }
    }
  }
  return ret;
}

Result Verify(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  Accumulate(&ret, VerifyAtLeastOneCapsule(cf));
  Accumulate(&ret, VerifyTypeSoundness(cf));
  Accumulate(&ret, VerifyNamesDistinctFromTypes(cf));
  return ret;
}

}  // namespace capsule
