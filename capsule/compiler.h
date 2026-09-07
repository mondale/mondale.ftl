#ifndef CAPSULE_COMPILER_H_
#define CAPSULE_COMPILER_H_

#include <string>
#include <string_view>

#include "core/vocabulary.h"

namespace capsule {

// Run compilation based on command-line flags.
Result Compile();

// Run compilation frontend on a file, checking for errors. Nominally a testing
// hook for negative compilation tests.
Result VerifyFrontendCompile(std::string_view capsule_file_name);

}  // namespace capsule

#endif  // #ifndef CAPSULE_COMPILER_H_
