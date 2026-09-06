#ifndef CAPSULE_GENERATOR_H_
#define CAPSULE_GENERATOR_H_

#include <string>
#include <string_view>
#include <vector>

#include "capsule/ast.h"
#include "core/vocabulary.h"

namespace capsule {

ResultOr<std::string> GenerateHeader(const CapsuleFile& file);
ResultOr<std::string> GenerateSource(const CapsuleFile& file,
                                     std::string_view header_location);

}  // namespace capsule

#endif  // CAPSULE_GENERATOR_H_
