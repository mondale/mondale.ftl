#ifndef CAPSULE_AST_H_
#define CAPSULE_AST_H_

#include <string>
#include <vector>

namespace capsule {

struct Attribute {
  std::string name;
  std::string value;
};

struct Field {
  std::string type;
  std::string name;
  std::vector<Attribute> attributes;
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
