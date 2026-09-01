#ifndef CAPSULE_DECODER_H_
#define CAPSULE_DECODER_H_

#include <bit>
#include <string_view>

#include "capsule/codec.h"
#include "capsule/view_mapper.h"
#include "core/vocabulary.h"

namespace capsule {

// Helper to decode a Storage to a View.
class Decoder final {
 public:
  Decoder(void* base);

  static ResultOr<Decoder> Build(void* base, size_t memory_length);

  ResultOr<bool> FindBoolean(core::CRC32C h) const {
    TRY_ASSIGN(const uint32_t v, vm_.Lookup(h));
    if ((v & 0xFFFFFFF0u) != 0xBBBBBBB0u) {
      return Code::kError;  // capsule fatal
    }
    if ((v & 0x0Fu) == 0x01) return true;
    if ((v & 0x0Fu) == 0x00) return false;
    return Code::kError;  // capsule fatal
  }

  ResultOr<uint8_t> FindU8(core::CRC32C h) const {
    TRY_ASSIGN(const uint32_t v, vm_.Lookup(h));
    if ((v & 0xFFFFFF00u) != 0x88888800u) {
      return Code::kError;  // capsule fatal
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
      return Code::kError;  // capsule fatal
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
    if (ptr > (length_ - 8)) return Code::kError;  // capsule fatal
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
    if (ptr > (length_ - 4)) return Code::kError;  // capsule fatal
    const uint32_t str_len = *Codec::U32AtOffset(base_, ptr);
    if ((ptr + 4 + str_len) > length_) return Code::kError;
    const char* const s =
        reinterpret_cast<const char*>(Codec::U32AtOffset(base_, ptr + 4));
    return std::string_view(s, str_len);
  }

  ResultOr<Decoder> FindCapsule(core::CRC32C h) const {
    TRY_ASSIGN(const uint32_t ptr, FindU32(h));
    if ((ptr + 4) > length_) return Code::kError;  // capsule fatal
    const uint32_t len = *Codec::U32AtOffset(base_, ptr + 4);
    return Build(const_cast<char*>(reinterpret_cast<const char*>(base_)) + ptr,
                 len);
  }

 private:
  void* const base_;
  size_t length_;
  ViewMapper vm_;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_DECODER_H_
