#include <cpuid.h>

#include <cstdint>

#include "base/time.h"

namespace base {
namespace {

// Queries hardware registers for exact TSC frequency in Hz.
// Returns 0 if unsupported by the CPU or running in an uncooperative VM.
uint64_t DetectCpuFrequencyHzX64() {
  uint64_t frequency_hz = 0;
#if defined(__x86_64__)
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

  // 1. Check max supported CPUID leaf
  if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx) || eax < 0x15) {
    return 0;
  }

  // 2. Query Leaf 0x15: Time Stamp Counter and Nominal Core Crystal Clock
  __cpuid_count(0x15, 0, eax, ebx, ecx, edx);

  const uint32_t denominator = eax;
  const uint32_t numerator = ebx;
  uint64_t crystal_hz = ecx;

  // If numerator or denominator is zero, Leaf 0x15 is unsupported.
  if (denominator == 0 || numerator == 0) {
    return 0;
  }

  // 3. Handle missing ECX (crystal frequency reported as 0 on some Intel
  // microarchitectures)
  if (crystal_hz == 0) {
    uint32_t max_leaf = 0;
    __get_cpuid(0, &max_leaf, &ebx, &ecx, &edx);

    if (max_leaf >= 0x16) {
      uint32_t base_mhz = 0;
      __cpuid_count(0x16, 0, base_mhz, ebx, ecx, edx);
      if (base_mhz > 0) {
        // Leaf 0x16 EAX returns base processor frequency in MHz
        return static_cast<uint64_t>(base_mhz) * 1'000'000ULL;
      }
    }
    return 0;
  }

  // 4. TSC Frequency = Crystal Clock * (Numerator / Denominator)
  frequency_hz = (crystal_hz * numerator) / denominator;
#endif
  return frequency_hz;
}

uint64_t DetectCpuFrequencyHzARM64() {
  uint64_t frequency_hz = 0;
#if defined(__aarch64__)
  uint64_t freq;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freq));
  frequency_hz = freq;
#endif
  return frequency_hz;
}

int64_t DetectCpuFrequencyHz() {
  return static_cast<int64_t>(DetectCpuFrequencyHzX64() +
                              DetectCpuFrequencyHzARM64());
}

int64_t GetCachedCpuFrequencyHz() {
  static int64_t memoized_frequency{DetectCpuFrequencyHz()};
  return memoized_frequency;
}

}  // namespace

int64_t CycleTime::CpuFrequencyHz() { return GetCachedCpuFrequencyHz(); }

}  // namespace base
