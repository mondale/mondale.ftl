#ifndef CAPSULE_DECODER_H_
#define CAPSULE_DECODER_H_

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
  Decoder(void* base);

  static ResultOr<Decoder> Build(void* base, size_t memory_length);

  // If this type is called, you need to define a specialization below.
  template <typename T>
  Code Find(core::CRC32C h, T* out, const T& def,
            std::vector<bool>::reference present) = delete;

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
                  std::vector<bool>::reference present) {
    return FindBoolean(h, out, def, present);
  }

  template <typename A32>
  Code Find32bPrimitive(core::CRC32C h, A32* out, const A32& def,
                        std::vector<bool>::reference present) const {
    uint32_t v = 0;
    const auto code = vm_.Lookup(h, &v);
    if (Code::kOk == code) {
      *out = std::bit_cast<A32>(v);
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
                     std::vector<bool>::reference present) {
    return Find32bPrimitive(h, out, def, present);
  }

  template <typename S>
  Code FindString(core::CRC32C h, S* out, const S& def,
                  std::vector<bool>::reference present) {
    uint32_t ptr = 0;
    const auto code = vm_.Lookup(h, &ptr);
    if (Code::kOk == code) {
      if (ptr > (length_ - 4)) return Code::kError;  // TODO capsule fatal
      const uint32_t str_len = *Codec::U32AtOffset(base_, ptr);
      if ((ptr + 4 + str_len) > length_) return Code::kError;
      const char* const s =
          reinterpret_cast<const char*>(Codec::U32AtOffset(base_, ptr + 4));
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
                         std::vector<bool>::reference present) {
    return FindString(h, out, def, present);
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
  [[maybe_unused]] void* const base_;
  size_t length_;
  ViewMapper vm_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_DECODER_H_
