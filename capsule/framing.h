#ifndef CAPSULE_FRAMING_H_
#define CAPSULE_FRAMING_H_

#include "core/vocabulary.h"

namespace capsule {

class Framing final {
 public:
  static Result Validate(void* base, size_t n);
  static Result Sign(void* base, size_t n);
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_FRAMING_H_
