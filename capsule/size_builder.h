#ifndef CAPSULE_SIZE_BUILDER_H_
#define CAPSULE_SIZE_BUILDER_H_

#include "capsule/abi.h"

namespace capsule {

class SizeBuilder final {
 public:
  SizeBuilder() = default;

  size_t Build() const {
    return payload_bytes_ +  // vars 8B and higher
           ote_count_ *
               sizeof(abi::OffsetTableEntry) +  // small vars & pointers
           sizeof(abi::FrameHeader) +           // header
           sizeof(core::CRC32C);                // integrity protection CRC
  }

  SizeBuilder& Add8bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add16bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add32bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add64bField() {  // encodes in the payload area
    ote_count_++;
    payload_bytes_ += 8;
    return *this;
  }

  SizeBuilder& AddVariablelengthField(size_t payload) {
    ote_count_++;
    payload_bytes_ += ((payload + 7) / 8 * 8);
    return *this;
  }

 private:
  int ote_count_ = 0;
  size_t payload_bytes_ = 0;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_SIZE_BUILDER_H_
