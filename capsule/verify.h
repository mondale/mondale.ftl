#ifndef CAPSULE_VERIFY_H_
#define CAPSULE_VERIFY_H_

#include "capsule/ast.h"
#include "core/vocabulary.h"

namespace capsule {

// Run all verifications possible on 'cf' without modifying it. Final step of
// frontend compilation before passing to code generation.
Result Verify(const CapsuleFile& cf);

// Other methods exposed for unit testing.
Result VerifyAtLeastOneCapsule(const CapsuleFile& cf);
Result VerifyTypeSoundness(const CapsuleFile& cf);
Result VerifyCapsuleNameUniqueness(const CapsuleFile& cf);
Result VerifyNamesDistinctFromTypes(const CapsuleFile& cf);

}  // namespace capsule

#endif  // #ifndef CAPSULE_VERIFY_H_
