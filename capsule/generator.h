#ifndef CAPSULE_GENERATOR_H_
#define CAPSULE_GENERATOR_H_

#include <string>
#include <vector>

#include "capsule/ast.h"
#include "core/vocabulary.h"

namespace capsule {

ResultOr<std::string> GenerateHeader(const CapsuleFile& file);

}  // namespace capsule

#endif  // CAPSULE_GENERATOR_H_
