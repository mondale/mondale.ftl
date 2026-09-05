#ifndef CAPSULE_DECODER_H_
#define CAPSULE_DECODER_H_

#include <string.h>

#include <bit>
#include <string_view>
#include <vector>

#include "capsule/codec.h"
#include "capsule/view_mapper.h"
#include "core/vocabulary.h"

namespace capsule {

// Helper to decode a Storage to a View or a Materialized.
class Decoder final {
 public:
  Decoder(const void* base);

  static ResultOr<Decoder> Build(const void* base, size_t memory_length);

  // If this type is called, you need to define a specialization below.
  template <typename T>
  Code Find(core::CRC32C h, T* out, const T& def,
            std::vector<bool>::reference present) const = delete;

  Code FindBoolean(core::CRC32C h, bool* out, const bool& def,
                   std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      if ((v & 0xFFFFFFFEu) != 0xBBBBBBB0u) {
        return Code::kCapsuleFatal;
      }
      *out = ((v & 0x01u) == 0x01);
      present = true;
      return Code::kOk;
    } else if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    }
    return code;
  }

  template <>
  Code Find<bool>(core::CRC32C h, bool* out, const bool& def,
                  std::vector<bool>::reference present) const {
    return FindBoolean(h, out, def, present);
  }

  template <typename A8>
  Code Find8bPrimitive(core::CRC32C h, A8* out, const A8& def,
                       std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      if ((v & 0xFFFFFF00u) != 0x88888800u) {
        return Code::kCapsuleFatal;
      }
      *out = static_cast<A8>(v & 0x0FFu);
      present = true;
      return Code::kOk;
    } else if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    }
    return code;
  }

  template <>
  Code Find<int8_t>(core::CRC32C h, int8_t* out, const int8_t& def,
                    std::vector<bool>::reference present) const {
    return Find8bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<uint8_t>(core::CRC32C h, uint8_t* out, const uint8_t& def,
                     std::vector<bool>::reference present) const {
    return Find8bPrimitive(h, out, def, present);
  }

  template <typename A16>
  Code Find16bPrimitive(core::CRC32C h, A16* out, const A16& def,
                        std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      if ((v & 0xFFFF0000u) != 0x16160000u) {
        return Code::kCapsuleFatal;
      }
      *out = static_cast<A16>(v & 0x0FFFFu);
      present = true;
      return Code::kOk;
    } else if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    }
    return code;
  }

  template <>
  Code Find<int16_t>(core::CRC32C h, int16_t* out, const int16_t& def,
                     std::vector<bool>::reference present) const {
    return Find16bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<uint16_t>(core::CRC32C h, uint16_t* out, const uint16_t& def,
                      std::vector<bool>::reference present) const {
    return Find16bPrimitive(h, out, def, present);
  }

  template <typename A32>
    requires(std::integral<A32> || std::same_as<A32, float>)
  A32 ProperCast32(uint32_t val) const {
    if constexpr (std::integral<A32>) {
      return static_cast<A32>(val);
    } else {
      return std::bit_cast<A32>(val);
    }
  }

  template <typename A32>
  Code Find32bPrimitive(core::CRC32C h, A32* out, const A32& def,
                        std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      *out = ProperCast32<A32>(v);
      present = true;
      return Code::kOk;
    } else if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    }
    return code;
  }

  template <>
  Code Find<int32_t>(core::CRC32C h, int32_t* out, const int32_t& def,
                     std::vector<bool>::reference present) const {
    return Find32bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<uint32_t>(core::CRC32C h, uint32_t* out, const uint32_t& def,
                      std::vector<bool>::reference present) const {
    return Find32bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<float>(core::CRC32C h, float* out, const float& def,
                   std::vector<bool>::reference present) const {
    return Find32bPrimitive(h, out, def, present);
  }

  template <typename A64>
    requires(std::integral<A64> || std::same_as<A64, double>)
  A64 ProperCast64(uint64_t val) const {
    if constexpr (std::integral<A64>) {
      return static_cast<A64>(val);
    } else {
      return std::bit_cast<A64>(val);
    }
  }

  template <typename A64>
  Code Find64bPrimitive(core::CRC32C h, A64* out, const A64& def,
                        std::vector<bool>::reference present) const {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    } else if (Code::kOk != code) {
      return code;
    }
    // Code is OK, indirect to get the 64b primitive.
    if (ptr > (length_ - 8)) return Code::kCapsuleFatal;
    if ((ptr % 8) != 0) return Code::kCapsuleFatal;
    *out = ProperCast64<A64>(*Codec::AtPtr<const uint64_t>(base_, ptr));
    present = true;
    return Code::kOk;
  }

  template <>
  Code Find<int64_t>(core::CRC32C h, int64_t* out, const int64_t& def,
                     std::vector<bool>::reference present) const {
    return Find64bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<uint64_t>(core::CRC32C h, uint64_t* out, const uint64_t& def,
                      std::vector<bool>::reference present) const {
    return Find64bPrimitive(h, out, def, present);
  }

  template <>
  Code Find<double>(core::CRC32C h, double* out, const double& def,
                    std::vector<bool>::reference present) const {
    return Find64bPrimitive(h, out, def, present);
  }

  template <typename S>
  Code FindString(core::CRC32C h, S* out, const S& def,
                  std::vector<bool>::reference present) const {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kOk == code) {
      if (ptr > (length_ - 4)) return Code::kCapsuleFatal;
      const uint32_t str_len = *Codec::AtPtr<const uint32_t>(base_, ptr);
      if (str_len > length_) return Code::kCapsuleFatal;
      if ((ptr + 4 + str_len) > length_) return Code::kCapsuleFatal;
      const char* const s = Codec::AtPtr<const char>(base_, ptr + 4);
      *out = std::string_view(s, str_len);
      present = true;
      return Code::kOk;
    } else if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    }
    return code;
  }

  template <>
  Code Find<std::string>(core::CRC32C h, std::string* out,
                         const std::string& def,
                         std::vector<bool>::reference present) const {
    return FindString(h, out, def, present);
  }

  template <>
  Code Find<std::string_view>(core::CRC32C h, std::string_view* out,
                              const std::string_view& def,
                              std::vector<bool>::reference present) const {
    return FindString(h, out, def, present);
  }

  template <typename S>
  void SetStringValueHelper(S* s, const char* c, uint32_t n) const = delete;

  template <>
  void SetStringValueHelper<std::string>(std::string* s, const char* c,
                                         uint32_t n) const {
    s->resize(0);
    s->append(c, n);
  }

  template <>
  void SetStringValueHelper<std::string_view>(std::string_view* s,
                                              const char* c, uint32_t n) const {
    *s = std::string_view(c, n);
  }

  template <typename S>
  Code FindStringVector(core::CRC32C h, std::vector<S>* out,
                        std::vector<bool>::reference present) const {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kNotFound == code) {
      out->resize(0);
      out->shrink_to_fit();
      present = false;
      return Code::kOk;
    } else if (Code::kOk != code) {
      return code;
    }

    if (ptr > (length_ - sizeof(abi::VectorHeader))) return Code::kCapsuleFatal;
    if ((ptr % 8) != 0) return Code::kCapsuleFatal;
    const auto* const vh = Codec::AtPtr<const abi::VectorHeader>(base_, ptr);
    if (vh->padding != 0xda4eda4eu) return Code::kCapsuleFatal;
    const uint32_t element_count = vh->element_count;
    out->resize(element_count);
    ptr += sizeof(abi::VectorHeader);
    present = true;

    for (uint32_t i = 0; i < element_count; ++i) {
      const uint32_t string_length = *Codec::AtPtr<const uint32_t>(base_, ptr);
      uint32_t str_ptr = ptr + sizeof(uint32_t);
      if (string_length > length_) return Code::kCapsuleFatal;
      if (str_ptr > (length_ - string_length)) return Code::kCapsuleFatal;
      SetStringValueHelper(
          &out->at(i), Codec::AtPtr<const char>(base_, str_ptr), string_length);
      ptr += (string_length + sizeof(uint32_t) + 7) / 8 * 8;
    }
    return Code::kOk;
  }

  template <typename C>
  Code FindCapsule(core::CRC32C h, C* out, const C& def,
                   std::vector<bool>::reference present) const {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kNotFound == code) {
      *out = def;
      present = false;
      return Code::kOk;
    } else if (Code::kOk != code) {
      return code;
    }
    if (ptr > (length_ - sizeof(abi::Header))) return Code::kCapsuleFatal;
    if ((ptr % 8) != 0) return Code::kCapsuleFatal;
    const void* const capsule_base = reinterpret_cast<const void*>(
        reinterpret_cast<const char*>(base_) + ptr);
    const uint32_t capsule_length =
        reinterpret_cast<const abi::Header*>(capsule_base)->capsule_length;
    if (capsule_length > (length_ - ptr)) return Code::kCapsuleFatal;
    auto mb = Build(capsule_base, capsule_length);
    if (!mb.ok()) {
      return mb.result().code();
    }
    present = true;
    return out->Decode(&mb.ValueOrDie()).code();
  }

  template <typename C>
  Code FindCapsuleVector(core::CRC32C h, std::vector<C>* out,
                         std::vector<bool>::reference present) const {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kNotFound == code) {
      out->clear();
      out->shrink_to_fit();
      present = false;
      return Code::kOk;
    } else if (Code::kOk != code) {
      return code;
    }
    if (ptr > (length_ - sizeof(abi::VectorHeader))) return Code::kCapsuleFatal;
    if ((ptr % 8) != 0) return Code::kCapsuleFatal;
    const auto* const vh = Codec::AtPtr<const abi::VectorHeader>(base_, ptr);
    if (vh->padding != 0xda4eda4eu) return Code::kCapsuleFatal;
    const uint32_t element_count = vh->element_count;
    out->resize(element_count);
    ptr += sizeof(abi::VectorHeader);
    present = true;

    for (uint32_t i = 0; i < element_count; ++i) {
      const auto* const h = Codec::AtPtr<const abi::Header>(base_, ptr);
      const uint32_t subcapsule_length = h->capsule_length;
      if (subcapsule_length > length_) return Code::kCapsuleFatal;
      if (ptr > (length_ - subcapsule_length)) return Code::kCapsuleFatal;
      auto mb = Build(h, subcapsule_length);
      if (!mb.ok()) {
        return mb.result().code();
      }
      const auto inner_code = out->at(i).Decode(&mb.ValueOrDie()).code();
      if (inner_code != Code::kOk) return inner_code;
      ptr += subcapsule_length;
    }
    return Code::kOk;
  }

 private:
  const void* const base_;
  size_t length_;
  ViewMapper vm_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_DECODER_H_
