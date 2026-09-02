#include "capsule/codec.h"
#include "capsule/encoder.h"

namespace capsule {

Encoder::Encoder(void* base, size_t length, uint32_t field_count, Encoder* p)
    : parent_(p),
      base_(base),
      payload_cursor_(sizeof(abi::Header) +
                      sizeof(abi::OffsetTableEntry) * field_count),
      payload_bytes_remain_(Codec::PayloadAreaSize(length, field_count)),
      field_cursor_(0),
      field_count_(field_count),
      encoding_result_() {
  *Codec::HeaderToOteCount(base) = field_count;
  *Codec::HeaderToLength(base) = length;
}

void Encoder::ErrorSpaceOverflow() {
  SetIfOk(
      Result(Code::kExhausted, "Payload overflow during capsule encoding."));
}

void Encoder::ErrorSlotOverflow() {
  SetIfOk(Result(Code::kExhausted, "Slot overflow during capsule encoding."));
}

void Encoder::SetIfOk(Result r) {
  if (IsOk(encoding_result_)) {
    encoding_result_ = r;
  }

  // Recurse errors to the topmost Encoder.
  Encoder* e = parent_;
  while (e != nullptr) {
    if (IsOk(e->encoding_result_)) {
      e->encoding_result_ = r;
    }
    e = e->parent_;
  }
}

}  // namespace capsule
