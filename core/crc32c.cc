#include <cstdint>

#include "core/crc32c.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define CORE_CRC32C_TARGET __attribute__((target("sse4.2,crc32")))
#elif defined(__aarch64__)
#if __has_include(<arm_acle.h>)
#include <arm_acle.h>
#endif
#define CORE_CRC32C_TARGET __attribute__((target("+crc")))
#else
#error "CRC32C implementation requires x64 or AArch64 hardware intrinsics."
#endif

namespace core {

CORE_CRC32C_TARGET CRC32C ComputeCRC32C(const void* p, size_t l, CRC32C v) {
  const auto* ptr = static_cast<const uint8_t*>(p);
  uint32_t crc = ~v.value();

#if defined(__x86_64__) || defined(_M_X64)
  while (l >= 8) {
    crc = static_cast<uint32_t>(
        _mm_crc32_u64(crc, *reinterpret_cast<const uint64_t*>(ptr)));
    ptr += 8;
    l -= 8;
  }
  if (l >= 4) {
    crc = _mm_crc32_u32(crc, *reinterpret_cast<const uint32_t*>(ptr));
    ptr += 4;
    l -= 4;
  }
  if (l >= 2) {
    crc = _mm_crc32_u16(crc, *reinterpret_cast<const uint16_t*>(ptr));
    ptr += 2;
    l -= 2;
  }
  if (l == 1) {
    crc = _mm_crc32_u8(crc, *ptr);
  }
#elif defined(__aarch64__)
  while (l >= 8) {
    crc = __builtin_arm_crc32d(crc, *reinterpret_cast<const uint64_t*>(ptr));
    ptr += 8;
    l -= 8;
  }
  if (l >= 4) {
    crc = __builtin_arm_crc32w(crc, *reinterpret_cast<const uint32_t*>(ptr));
    ptr += 4;
    l -= 4;
  }
  if (l >= 2) {
    crc = __builtin_arm_crc32h(crc, *reinterpret_cast<const uint16_t*>(ptr));
    ptr += 2;
    l -= 2;
  }
  if (l == 1) {
    crc = __builtin_arm_crc32b(crc, *ptr);
  }
#endif

  return CRC32C(~crc);
}

}  // namespace core
