#ifndef CAPSULE_ENCODER_H_
#define CAPSULE_ENCODER_H_

#include <string.h>

#include <bit>

#include "capsule/codec.h"
#include "capsule/size_builder.h"
#include "core/vocabulary.h"

namespace capsule {
namespace internal {

template <typename T>
concept HasEncode = requires(T t) { t.Encode(nullptr); };

}  // namespace internal

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

  template <>
  void Add<uint64_t>(core::CRC32C hash, const uint64_t& v) {
    AddU64(hash, v);
  }

  template <>
  void Add<int64_t>(core::CRC32C hash, const int64_t& v) {
    AddI64(hash, v);
  }

  template <>
  void Add<double>(core::CRC32C hash, const double& v) {
    AddF64(hash, v);
  }

  template <>
  void Add<uint32_t>(core::CRC32C hash, const uint32_t& v) {
    AddU32(hash, v);
  }

  template <>
  void Add<int32_t>(core::CRC32C hash, const int32_t& v) {
    AddI32(hash, v);
  }

  template <>
  void Add<float>(core::CRC32C hash, const float& v) {
    AddF32(hash, v);
  }

  template <>
  void Add<uint16_t>(core::CRC32C hash, const uint16_t& v) {
    AddU16(hash, v);
  }

  template <>
  void Add<int16_t>(core::CRC32C hash, const int16_t& v) {
    AddI16(hash, v);
  }

  template <>
  void Add<uint8_t>(core::CRC32C hash, const uint8_t& v) {
    AddU8(hash, v);
  }

  template <>
  void Add<int8_t>(core::CRC32C hash, const int8_t& v) {
    AddI8(hash, v);
  }

  template <>
  void Add<bool>(core::CRC32C hash, const bool& b) {
    AddBoolean(hash, b);
  }

  void AddU32(core::CRC32C hash, uint32_t value) {
    if (field_cursor_ >= field_count_) {
      DVLOG(1) << "Error location";
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

  void AddU64(core::CRC32C hash, uint64_t value) {
    if (payload_bytes_remain_ < 8) {
      DVLOG(1) << "Error location";
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
      DVLOG(1) << "Error location";
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

  // Helper for adding a capsule.
  template <typename T>
    requires internal::HasEncode<T>
  void Add(core::CRC32C hash, const T& t) {
    const uint32_t capsule_length = t.ComputeStorageSize();
    const uint32_t field_count = T::kFieldCount;
    auto e = AddCapsule(hash, capsule_length, field_count);
    t.Encode(&e);
  }

  Encoder AddCapsule(core::CRC32C hash, uint32_t len, uint32_t field_count) {
    if (len > payload_bytes_remain_) {
      DVLOG(1) << "Error location";
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
  void AddCapsuleVector(core::CRC32C hash, const std::vector<T>& v) {
    // Calculate space needed for full capsule vector in payload area.
    size_t payload_size = sizeof(abi::VectorHeader);
    for (const auto& c : v) {
      payload_size += c.ComputeStorageSize();
    }

    if (payload_bytes_remain_ < payload_size) {
      DVLOG(1) << "Error location";
      ErrorSpaceOverflow();
      return;
    }

    AddU32(hash, payload_cursor_);

    // Encode the vector header.
    char* dst = reinterpret_cast<char*>(base_) + payload_cursor_;
    auto* const vh = reinterpret_cast<abi::VectorHeader*>(dst);
    vh->element_count = static_cast<uint32_t>(v.size());
    vh->padding = 0xda4ada4eu;
    payload_cursor_ += sizeof(abi::VectorHeader);
    payload_bytes_remain_ -= sizeof(abi::VectorHeader);

    // Encode each subordinate Capsule.
    const uint32_t subcapsule_field_count = T::kFieldCount;
    const core::CRC32C subcapsule_hash = T::kTypeHash;
    for (const auto& c : v) {
      const uint32_t subcapsule_length = c.ComputeStorageSize();

      auto e = AddCapsule(subcapsule_hash, subcapsule_length,
                          subcapsule_field_count);
      c.Encode(&e);
    }
  }

  template <typename T>
  void AddPrimitiveVector(core::CRC32C hash, const std::vector<T>& v) {
    const uint32_t num_elements = v.size();
    const uint32_t data_bytes = num_elements * sizeof(T);
    const uint32_t payload_size = (sizeof(uint32_t) + data_bytes + 7) / 8 * 8;
    const uint32_t padding = payload_size - sizeof(uint32_t) - data_bytes;
    const char* const src = reinterpret_cast<const char*>(v.data());
    if (payload_bytes_remain_ < payload_size) {
      DVLOG(1) << "Error location";
      ErrorSpaceOverflow();
      return;
    }
    AddU32(hash, payload_cursor_);

    // Encode number of elements;
    char* dst = reinterpret_cast<char*>(base_) + payload_cursor_;
    *reinterpret_cast<uint32_t*>(dst) = num_elements;
    dst += sizeof(uint32_t);

    // Encode the elements themselves.
    memcpy(dst, src, data_bytes);
    dst += data_bytes;

    // Encode the padding to round up to a mulitple of 8.
    memset(dst, 0xda, padding);
    payload_cursor_ += payload_size;
    payload_bytes_remain_ -= payload_size;
  }

  template <>
  void Add<std::vector<uint8_t>>(core::CRC32C hash,
                                 const std::vector<uint8_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<int8_t>>(core::CRC32C hash,
                                const std::vector<int8_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<uint16_t>>(core::CRC32C hash,
                                  const std::vector<uint16_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<int16_t>>(core::CRC32C hash,
                                 const std::vector<int16_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<uint32_t>>(core::CRC32C hash,
                                  const std::vector<uint32_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<int32_t>>(core::CRC32C hash,
                                 const std::vector<int32_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<uint64_t>>(core::CRC32C hash,
                                  const std::vector<uint64_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<int64_t>>(core::CRC32C hash,
                                 const std::vector<int64_t>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<float>>(core::CRC32C hash, const std::vector<float>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<double>>(core::CRC32C hash,
                                const std::vector<double>& v) {
    AddPrimitiveVector(hash, v);
  }

  template <>
  void Add<std::vector<std::string>>(core::CRC32C hash,
                                     const std::vector<std::string>& v) {
    // Every string has an individual byte length, and each string's encoding
    // is rounded up to a multiple of 8.
    size_t payload_size = sizeof(abi::VectorHeader);
    for (const auto& s : v) {
      payload_size += (4 + s.length() + 7) / 8 * 8;
    }
    if (payload_bytes_remain_ < payload_size) {
      DVLOG(1) << "Error location";
      ErrorSpaceOverflow();
      return;
    }

    AddU32(hash, payload_cursor_);

    // Encode the vector header.
    char* dst = reinterpret_cast<char*>(base_) + payload_cursor_;
    auto* const vh = reinterpret_cast<abi::VectorHeader*>(dst);
    vh->element_count = static_cast<uint32_t>(v.size());
    vh->padding = 0xda4ada4eu;
    dst += sizeof(abi::VectorHeader);

    // Encode the individual elements.
    for (const auto& s : v) {
      // Encode the string's length.
      const uint32_t length = s.length();
      *reinterpret_cast<uint32_t*>(dst) = length;
      dst += sizeof(uint32_t);

      // Encode the string's bytes.
      memcpy(dst, s.data(), length);
      dst += length;

      // Encode padding.
      memset(dst, 0xda, (length + 4) % 8);
    }

    payload_cursor_ += payload_size;
    payload_bytes_remain_ -= payload_size;
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
