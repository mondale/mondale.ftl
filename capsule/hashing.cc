#include <list>
#include <map>
#include <set>
#include <string>

#include "capsule/hashing.h"
#include "core/strings.h"

namespace capsule {
namespace {

Result MakeError(const Field& f, const std::string& msg) {
  std::string error = f.srcloc + ": " + msg;
  return Result(Code::kPrecondition, error);
}

template <typename F>
  requires std::invocable<F, const std::string&> &&
           std::same_as<std::invoke_result_t<F, const std::string&>, Result>
Result ForEachAttribute(const std::vector<Attribute>& as,
                        std::string_view named, F&& callback) {
  for (const auto& a : as) {
    if (a.name == named) {
      TRY(callback(a.value));
    }
  }
  return Result::Ok();
}

Result ComputeHashes(Field* f) {
  auto& v = f->hashes;

  // This had better already be empty.
  if (!v.empty()) {
    return MakeError(
        *f,
        strings::Format("'hashes' already contains [{}] hashes.", v.size()));
  }

  // Hash field name.
  v.push_back(core::ComputeCRC32C(f->name));

  // Find legacy field names and hash them too.
  TRY(ForEachAttribute(f->attributes, "former_name",
                       [&v](const std::string& former_name) {
                         v.push_back(core::ComputeCRC32C(former_name));
                         return Result::Ok();
                       }));
  return Result::Ok();
}

Result ComputeHashes(Capsule* c) {
  for (auto& f : c->fields) {
    TRY(ComputeHashes(&f));
  }
  return Result::Ok();
}

Result ComputeHashes(CapsuleFile* cf) {
  for (auto& c : cf->capsules) {
    TRY(ComputeHashes(&c));
  }
  return Result::Ok();
}

std::vector<std::string> FindCollisionsWith(core::CRC32C h, const Field& f) {
  std::vector<std::string> collisions;
  if (core::ComputeCRC32C(f.name) == h) collisions.push_back(f.name);
  for (const auto& a : f.attributes) {
    if (a.name != "former_name") continue;
    if (core::ComputeCRC32C(a.value) == h) collisions.push_back(a.value);
  }
  return collisions;
}

void AddDuplicateHash(core::CRC32C h, std::list<std::string>* complaints,
                      const Field* f1, const Field* f2) {
  std::string complaint;
  if (f1 == f2) {
    const std::vector<std::string> collisions = FindCollisionsWith(h, *f1);
    complaint = strings::Format(
        "{}: Field [{}] has at least one alias that collides with name [{}].",
        f1->srcloc, f1->name, strings::Join(collisions, ","));
  } else {
    std::set<std::string> collisions;
    auto c1 = FindCollisionsWith(h, *f1);
    auto c2 = FindCollisionsWith(h, *f2);
    for (const auto& s : c1) collisions.insert(s);
    for (const auto& s : c2) collisions.insert(s);
    complaint = strings::Format(
        "{}: Fields [{}] and [{}] have names or alias that collide [{}].",
        f1->srcloc, f1->name, f2->name, strings::Join(collisions, ","));
  }
  complaints->push_back(std::move(complaint));
}

Result ValidateHashes(const Capsule& c, std::list<std::string>* complaints) {
  // There cannot be any duplicate hashes belonging to any field inside any
  // individual capsule type.
  std::map<core::CRC32C, const Field*> m;
  for (const auto& f : c.fields) {
    if (f.hashes.empty()) {
      return MakeError(f, "No hashses present.");
    }
    for (const auto h : f.hashes) {
      auto iter = m.find(h);
      if (iter == m.end()) {
        m.insert(std::make_pair(h, &f));
        continue;
      }
      AddDuplicateHash(h, complaints, iter->second, &f);
    }
  }
  return Result::Ok();
}

Result ValidateHashes(const CapsuleFile& cf) {
  std::list<std::string> complaints;
  for (const auto& c : cf.capsules) {
    TRY(ValidateHashes(c, &complaints));
  }

  // Fold all complaints into a single status.
  if (complaints.empty()) return Result::Ok();
  return Result(Code::kInvalidArgument, strings::Join(complaints, "\n"));
}

}  // namespace

Result ComputeAndValidateHashes(CapsuleFile* cf) {
  TRY(ComputeHashes(cf));
  return ValidateHashes(*cf);
}

}  // namespace capsule
