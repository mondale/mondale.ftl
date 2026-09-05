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
        return Code::kError;  // TODO capsule fatal
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
        return Code::kError;  // TODO capsule fatal
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
        return Code::kError;  // TODO capsule fatal
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
  Code Find32bPrimitive(core::CRC32C h, A32* out, const A32& def,
                        std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      *out = static_cast<A32>(v);
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
    if (ptr > (length_ - 8)) return Code::kError;  // TODO capsule fatal
    if ((ptr % 8) != 0) return Code::kError;       // TODO capsule fatal
    *out = *Codec::AtPtr<const uint64_t>(base_, ptr);
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
      if (ptr > (length_ - 4)) return Code::kError;  // TODO capsule fatal
      const uint32_t str_len = *Codec::AtPtr<const uint32_t>(base_, ptr);
      if ((ptr + 4 + str_len) > length_) return Code::kError;
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

  Code FindStringVector(core::CRC32C h, std::vector<std::string>* out,
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

    if (ptr > (length_ - sizeof(abi::VectorHeader)))
      return Code::kError;                    // TODO capsule fatal
    if ((ptr % 8) != 0) return Code::kError;  // TODO capsule fatal
    const auto* const vh = Codec::AtPtr<const abi::VectorHeader>(base_, ptr);
    if (vh->padding != 0xda4eda4eu) return Code::kError;  // TODO capsule fatal
    const uint32_t element_count = vh->element_count;
    out->resize(element_count);
    ptr += sizeof(abi::VectorHeader);
    present = true;

    for (uint32_t i = 0; i < element_count; ++i) {
      const uint32_t string_length = *Codec::AtPtr<const uint32_t>(base_, ptr);
      uint32_t str_ptr = ptr + sizeof(uint32_t);
      if (string_length > length_) return Code::kError;  // TODO capsule fatal
      if (str_ptr > (length_ - string_length))
        return Code::kError;  // TODO capsule fatal
      std::string& s = out->at(i);
      s.resize(0);
      s.append(Codec::AtPtr<const char>(base_, str_ptr), string_length);
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
    if (ptr > (length_ - sizeof(abi::Header)))
      return Code::kError;                    // TODO capsule fatal
    if ((ptr % 8) != 0) return Code::kError;  // TODO capsule fatal
    const void* const capsule_base = reinterpret_cast<const void*>(
        reinterpret_cast<const char*>(base_) + ptr);
    const uint32_t capsule_length =
        reinterpret_cast<const abi::Header*>(capsule_base)->capsule_length;
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
    if (ptr > (length_ - sizeof(abi::VectorHeader)))
      return Code::kError;                    // TODO capsule fatal
    if ((ptr % 8) != 0) return Code::kError;  // TODO capsule fatal
    const auto* const vh = Codec::AtPtr<const abi::VectorHeader>(base_, ptr);
    if (vh->padding != 0xda4eda4eu) return Code::kError;  // TODO capsule fatal
    const uint32_t element_count = vh->element_count;
    out->resize(element_count);
    ptr += sizeof(abi::VectorHeader);
    present = true;

    for (uint32_t i = 0; i < element_count; ++i) {
      const auto* const h = Codec::AtPtr<const abi::Header>(base_, ptr);
      const uint32_t subcapsule_length = h->capsule_length;
      if (subcapsule_length > length_)
        return Code::kError;  // TODO capsule fatal
      if (ptr > (length_ - subcapsule_length))
        return Code::kError;  // TODO capsule fatal
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

  /*
    ResultOr<uint8_t> FindU8(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t v, vm_.Lookup(h));
      if ((v & 0xFFFFFF00u) != 0x88888800u) {
        return Code::kError;  // TODO capsule fatal
      }
      return static_cast<uint8_t>(v & 0x0FFu);
    }

    ResultOr<int8_t> FindI8(core::CRC32C h) const {
      TRY_ASSIGN(const uint8_t v, FindU8(h));
      return std::bit_cast<int8_t>(v);
    }

    ResultOr<uint16_t> FindU16(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t v, vm_.Lookup(h));
      if ((v & 0xFFFF0000u) != 0x16160000u) {
        return Code::kError;  // TODO capsule fatal
      }
      return static_cast<uint16_t>(v & 0x0FFFFu);
    }

    ResultOr<int16_t> FindI16(core::CRC32C h) const {
      TRY_ASSIGN(const uint16_t v, FindU16(h));
      return std::bit_cast<int16_t>(v);
    }

    ResultOr<uint32_t> FindU32(core::CRC32C h) const { return vm_.Lookup(h); }

    ResultOr<int32_t> FindI32(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t v, FindU32(h));
      return std::bit_cast<int32_t>(v);
    }

    ResultOr<float> FindF32(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t v, FindU32(h));
      return std::bit_cast<float>(v);
    }

    ResultOr<uint64_t> FindU64(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t ptr, FindU32(h));
      if (ptr > (length_ - 8)) return Code::kError;  // TODO capsule fatal
      return *reinterpret_cast<uint64_t*>(Codec::U32AtOffset(base_, ptr));
    }

    ResultOr<int64_t> FindI64(core::CRC32C h) const {
      TRY_ASSIGN(const uint64_t v, FindU64(h));
      return std::bit_cast<int64_t>(v);
    }

    ResultOr<double> FindF64(core::CRC32C h) const {
      TRY_ASSIGN(const uint64_t v, FindU64(h));
      return std::bit_cast<double>(v);
    }

    ResultOr<std::string_view> FindString(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t ptr, FindU32(h));
      if (ptr > (length_ - 4)) return Code::kError;  // TODO capsule fatal
      const uint32_t str_len = *Codec::U32AtOffset(base_, ptr);
      if ((ptr + 4 + str_len) > length_) return Code::kError;
      const char* const s =
          reinterpret_cast<const char*>(Codec::U32AtOffset(base_, ptr + 4));
      return std::string_view(s, str_len);
    }

    ResultOr<Decoder> FindCapsule(core::CRC32C h) const {
      TRY_ASSIGN(const uint32_t ptr, FindU32(h));
      if ((ptr + 4) > length_) return Code::kError;  // TODO capsule fatal
      const uint32_t len = *Codec::U32AtOffset(base_, ptr + 4);
      return Build(const_cast<char*>(reinterpret_cast<const char*>(base_)) +
    ptr, len);
    }

  */
 private:
  const void* const base_;
  size_t length_;
  ViewMapper vm_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_DECODER_H_
