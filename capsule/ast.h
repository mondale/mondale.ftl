#ifndef CAPSULE_AST_H_
#define CAPSULE_AST_H_

#include <string>
#include <vector>

#include "core/crc32c.h"

namespace capsule {

struct Attribute {
  std::string name;
  std::string value;
};

struct Field {
  std::string type;
  std::string name;
  std::string srcloc;
  std::vector<Attribute> attributes;
  std::vector<core::CRC32C> hashes;
};

struct Capsule {
  std::string name;
  std::vector<Field> fields;
};

struct CapsuleFile {
  std::string namespace_name;
  std::vector<Capsule> capsules;
};

}  // namespace capsule

#endif  // CAPSULE_AST_H_
