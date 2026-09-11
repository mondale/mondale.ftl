#include <list>
#include <set>
#include <unordered_set>

#include "capsule/verify.h"

using strings::Format;

namespace capsule {
namespace {

std::unordered_set<std::string_view> VerbotenSet() {
  std::unordered_set<std::string_view> r;
  static constexpr const char* kNope[] = {
      // clang-format off
  "int", "char", "long", "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t",
  "size_t", "uintptr_t", "intptr_t", "string", "Result", "core", "base", "Log", "capsule", "vector", "string_view",
  "const", "mutable", "override", "final", "class", "struct", "public", "private", "protected", "operator",
  "template", "typename", "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
  "case", "catch", "concept", "constexpr", "consteval", "continue", "delete", "do", "double", "float", "if", "for",
  "else", "explicit", "goto", "inline", "new", "return", "requires", "signed", "static", "switch", "this", "true",
  "false", "try", "union", "unsigned", "virtual", "void", "volatile", "namespace", "kTypeHash", "has_", "Decode",
  "Encode", "ComputeStorageSize", "RefIfNeeded", "CRC32C", "kFieldCount", "MaterializedType", "ViewType", "has",
      // clang-format on
  };
  for (auto x : kNope) r.insert(x);
  return r;
}

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

bool IsCapsuleType(std::string_view t) {
  return !IsPrimitiveType(t) && !IsVectorType(t);
}

std::string VectorToInner(std::string_view t) {
  if (t.length() < 7) return "";
  if (t.rfind("vector<", 0) == 0) {
    return std::string(t.substr(7, t.size() - 8));
  }
  return "";
}

Result Recognize(const std::string& srcloc, const Attribute& a) {
  if (a.name == "default") {
    if (a.value.empty()) {
      return Error(srcloc, Format("Attribute @default requires a value."));
    }
    return Result::Ok();
  }
  if (a.name == "formerly") {
    if (a.value.empty()) {
      return Error(srcloc, Format("Attribute @formerly requires a value."));
    }
    return Result::Ok();
  }
  if (a.name == "retired") {
    if (!a.value.empty()) {
      return Error(srcloc, Format("Attribute @retired does not take a value."));
    }
    return Result::Ok();
  }
  return Error(srcloc, Format("Unrecognized attribute [{}]", a.name));
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

Result VerifyCapsuleNameUniqueness(const CapsuleFile& cf) {
  Result ret = Result::Ok();

  std::set<std::string> capsule_types;
  for (const auto& c : cf.capsules) {
    if (capsule_types.find(c.name) == capsule_types.end()) {
      capsule_types.insert(c.name);
      continue;
    }
    Accumulate(&ret, Error(c.srcloc, Format("Capsule names must be unique "
                                            "within a capsule definition [{}].",
                                            c.name)));
  }
  return ret;
}

Result VerifyNamesNotVerboten(const CapsuleFile& cf) {
  Result ret = Result::Ok();

  // Various C++ reserved words and project vocabulary are verboten.
  const auto verboten = VerbotenSet();
  auto is_verboten = [&](const std::string& n) -> bool {
    return verboten.find(n) != verboten.end();
  };

  // Don't forget the damn namespace.
  if (is_verboten(cf.namespace_name)) {
    Accumulate(&ret, Error(-1, Format("Namespace name [{}] is verboten.",
                                      cf.namespace_name)));
  }

  // Capsule names may not be verboten.
  for (const auto& c : cf.capsules) {
    if (is_verboten(c.name)) {
      Accumulate(&ret, Error(c.srcloc,
                             Format("Capsule name [{}] is verboten.", c.name)));
    }
    for (const auto& f : c.fields) {
      if (is_verboten(f.name)) {
        Accumulate(&ret, Error(f.srcloc,
                               Format("Field name [{}] is verboten.", f.name)));
      }
    }
  }

  return ret;
}

Result VerifyNoGeneratedNameCollision(const CapsuleFile& cf) {
  Result ret = Result::Ok();

  // From capsule names, we'll create symbols:
  // CapsuleM, CapsuleV, CapsuleBase
  //
  // These types must be unique within the namespace, so these may not be used
  // as the name for fields. And while it would probably work, it'd be weird to
  // use those for capsules, so disallow that too.
  std::set<std::string> nope;
  auto is_disallowed = [&](const std::string& n) -> bool {
    return nope.find(n) != nope.end();
  };

  for (const auto& c : cf.capsules) {
    nope.insert(c.name + "M");
    nope.insert(c.name + "V");
    nope.insert(c.name + "Base");
  }
  for (const auto& c : cf.capsules) {
    if (is_disallowed(c.name)) {
      Accumulate(&ret,
                 Error(c.srcloc,
                       Format("Capsule name [{}] collides with generated name.",
                              c.name)));
    }
    for (const auto& f : c.fields) {
      if (is_disallowed(f.name)) {
        Accumulate(&ret,
                   Error(f.srcloc,
                         Format("Field name [{}] collides with generated name.",
                                f.name)));
      }
    }
  }

  nope.clear();  // reusing since it has such a nice name.

  // From field names, we'll generate additional symbols:
  // _Default, _Index, _FieldHash, has_foo
  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      nope.insert(f.name + "_Default");
      nope.insert(f.name + "_Index");
      nope.insert(f.name + "_FieldHash");
      nope.insert("has_" + f.name);
    }
  }
  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      if (is_disallowed(f.name)) {
        Accumulate(&ret,
                   Error(f.srcloc,
                         Format("Field name [{}] collides with generated name.",
                                f.name)));
      }
    }
  }

  return ret;
}

Result VerifyRecognizedAttributes(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      std::set<std::string> as;
      for (const auto& a : f.attributes) {
        Accumulate(&ret, Recognize(f.srcloc, a));
        if (as.find(a.name) != as.end()) {
          Accumulate(&ret, Error(f.srcloc,
                                 Format("Attribute [{}] appears redundantly.",
                                        a.name)));
        }
        as.insert(a.name);
      }
    }
  }
  return ret;
}

Result VerifyNoDefaultsOnVectorsOrCapsules(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  auto has_default = [](const Field& f) -> bool {
    for (const auto& a : f.attributes) {
      if (a.name == "default") return true;
    }
    return false;
  };

  for (const auto& c : cf.capsules) {
    for (const auto& f : c.fields) {
      if (!has_default(f)) continue;
      if (IsVectorType(f.type) || IsCapsuleType(f.type)) {
        Accumulate(
            &ret, Error(f.srcloc,
                        Format("@default attributes are nonsense on vector and "
                               "capsule types, e.g., [{}]",
                               f.type)));
      }
    }
  }
  return ret;
}

Result Verify(const CapsuleFile& cf) {
  Result ret = Result::Ok();
  Accumulate(&ret, VerifyAtLeastOneCapsule(cf));
  Accumulate(&ret, VerifyTypeSoundness(cf));
  Accumulate(&ret, VerifyCapsuleNameUniqueness(cf));
  Accumulate(&ret, VerifyNamesDistinctFromTypes(cf));
  Accumulate(&ret, VerifyNamesNotVerboten(cf));
  Accumulate(&ret, VerifyNoGeneratedNameCollision(cf));
  Accumulate(&ret, VerifyRecognizedAttributes(cf));
  Accumulate(&ret, VerifyNoDefaultsOnVectorsOrCapsules(cf));
  return ret;
}

}  // namespace capsule
