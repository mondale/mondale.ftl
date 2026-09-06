#ifndef CAPSULE_HASHING_H_
#define CAPSULE_HASHING_H_

#include "capsule/ast.h"
#include "core/crc32c.h"
#include "core/vocabulary.h"

namespace capsule {

Result ComputeAndValidateHashes(CapsuleFile* cf);

}  // namespace capsule

#endif  // #ifndef CAPSULE_HASHING_H_
