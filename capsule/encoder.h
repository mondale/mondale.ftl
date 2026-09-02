#ifndef CAPSULE_ENCODER_H_
#define CAPSULE_ENCODER_H_

#include <string.h>

#include <bit>

#include "capsule/codec.h"
#include "core/vocabulary.h"

namespace capsule {

// Encodes a capsule.
class Encoder final {
 public:
  Encoder(void* base, size_t length, uint32_t field_count,
          Encoder* p = nullptr);

  const Result& result() const { return encoding_result_; }

  // Returns the capsule length.
  uint32_t Seal() {
    *Codec::HeaderToLength(base_) = payload_cursor_;
    return payload_cursor_;
  }

  // If this is called, you need to add a template specialization below.
  template <typename T>
  void Add(core::CRC32C hash, const T& t) = delete;

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

  template <>
  void Add<int32_t>(core::CRC32C hash, const int32_t& v) {
    AddI32(hash, v);
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

  template <>
  void Add<bool>(core::CRC32C hash, const bool& b) {
    AddBoolean(hash, b);
  }

  void AddBoolean(core::CRC32C hash, bool value) {
    const uint32_t v32 = 0xBBBBBBB0u | (value ? 0x1u : 0x0u);
    AddU32(hash, v32);
  }

  template <>
  void Add<uint64_t>(core::CRC32C hash, const uint64_t& v) {
    AddU64(hash, v);
  }

  void AddU64(core::CRC32C hash, uint64_t value) {
    if (payload_bytes_remain_ < 8) {
      ErrorSpaceOverflow();
      return;
    }
    AddU32(hash, payload_cursor_);
    auto* const dst = reinterpret_cast<uint64_t*>(
        reinterpret_cast<char*>(base_) + payload_cursor_);
    *dst = value;
    payload_cursor_ += 8;
    payload_bytes_remain_ -= 8;
  }

  void AddI64(core::CRC32C hash, int64_t value) {
    AddU64(hash, std::bit_cast<uint64_t>(value));
  }

  void AddF64(core::CRC32C hash, double value) {
    AddU64(hash, std::bit_cast<uint64_t>(value));
  }

  template <>
  void Add<std::string>(core::CRC32C hash, const std::string& s) {
    AddString(hash, s.data(), s.length());
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

  template <typename T>
  void Add(core::CRC32C hash, const std::vector<T>& v) {
    // TODO - WORKING HERE
    // payload_bytes_ += 8;  // for vector length and type hash
    for ([[maybe_unused]] const auto& item : v) {
      // Add(item);  // needs to add an item of type 'hash'
    }
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
