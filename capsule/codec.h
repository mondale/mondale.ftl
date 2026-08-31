#ifndef CAPSULE_CODEC_H_
#define CAPSULE_CODEC_H_

#include "core/vocabulary.h"

namespace capsule {

class Codec final {
 public:
  static Result Validate(void* base, size_t n);
  static Result Sign(void* base, size_t n);

 private:
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_CODEC_H_
