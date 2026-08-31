#ifndef CAPSULE_ENCODER_H_
#define CAPSULE_ENCODER_H_

#include <string.h>

#include <bit>

#include "capsule/codec.h"
#include "core/vocabulary.h"

namespace capsule {

// Encodes an inner capsule.
class Encoder final {
 public:
  Encoder(void* base, size_t length, uint32_t field_count,
          Encoder* p = nullptr);

  const Result& result() const { return encoding_result_; }

  // Returns the inner capsule length.
  uint32_t Seal() {
    *Codec::InnerHeaderToLength(base_) = payload_cursor_;
    return payload_cursor_;
  }

  void AddU32(core::CRC32C hash, uint32_t value) {
    if (field_cursor_ >= field_count_) {
      ErrorSlotOverflow();
      return;
    }
    auto* const ote = Codec::OteEntryNumber(base_, field_cursor_);
    field_cursor_++;
    ote->field_hash = hash;
    ote->value = value;
  }

  void AddI32(core::CRC32C hash, int32_t value) {
    AddU32(hash, std::bit_cast<uint32_t>(value));
  }

  void AddF32(core::CRC32C hash, float value) {
    AddU32(hash, std::bit_cast<uint32_t>(value));
  }

  void AddU16(core::CRC32C hash, uint16_t value) {
    AddU32(hash, 0x16160000u | static_cast<uint32_t>(value));
  }

  void AddI16(core::CRC32C hash, int16_t value) {
    AddU16(hash, std::bit_cast<uint16_t>(value));
  }

  void AddU8(core::CRC32C hash, uint8_t value) {
    AddU32(hash, 0x88888800u | static_cast<uint32_t>(value));
  }

  void AddI8(core::CRC32C hash, int8_t value) {
    AddU8(hash, std::bit_cast<uint8_t>(value));
  }

  void AddBoolean(core::CRC32C hash, bool value) {
    const uint32_t v32 = 0xBBBBBBB0u | (value ? 0x1u : 0x0u);
    AddU32(hash, v32);
  }

  void AddString(core::CRC32C hash, const char* src, uint32_t len) {
    const uint32_t with_taxes = (4 + len + 7) / 8 * 8;
    const uint32_t taxes = with_taxes - len;
    if (with_taxes > payload_bytes_remain_) {
      ErrorSpaceOverflow();
      return;
    }
    AddU32(hash, payload_cursor_);
    char* const dst = reinterpret_cast<char*>(base_) + payload_cursor_;
    *reinterpret_cast<uint32_t*>(dst) = len;
    memcpy(dst + 4, src, len);
    memset(dst + 4 + len, 0xda, taxes);
    payload_cursor_ += with_taxes;
    payload_bytes_remain_ -= with_taxes;
  }

  Encoder AddCapsule(core::CRC32C hash, uint32_t len, uint32_t field_count) {
    if (len > payload_bytes_remain_) {
      ErrorSpaceOverflow();
      return Encoder(base_, len, field_count);
    }
    AddU32(hash, payload_cursor_);
    char* const dst = reinterpret_cast<char*>(base_) + payload_cursor_;
    payload_cursor_ += len;
    payload_bytes_remain_ -= len;
    return Encoder(dst, len, field_count, this);
  }

 private:
  void ErrorSpaceOverflow();
  void ErrorSlotOverflow();
  void SetIfOk(Result r);

  Encoder* const parent_;
  void* const base_;
  uint32_t payload_cursor_;
  uint32_t payload_bytes_remain_;
  uint32_t field_cursor_;
  const uint32_t field_count_;
  Result encoding_result_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_ENCODER_H_
