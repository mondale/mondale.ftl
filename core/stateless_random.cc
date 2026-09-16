#include "core/stateless_random.h"
#include "core/vocabulary.h"

namespace core {
namespace {

uint64_t Seed() {
  return CycleTime::Now().value() ^ (GetCachedTid() * 0x9e3779b97f4a7c15ull);
}

uint64_t Mix(uint64_t seed, uint64_t entropy) {
  uint64_t x = entropy ^ seed;
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

uint32_t ScaleToRange(uint32_t r, uint32_t range) {
  return static_cast<uint32_t>((static_cast<uint64_t>(r) * range) >> 32);
}

}  // namespace

uint64_t Rand64(uint64_t entropy) { return Mix(Seed(), entropy); }

ResultOr<size_t> WeightedSelect(std::span<const uint32_t> weights) {
  if (weights.empty()) {
    return core::InvalidArgumentError("Weights cannot be empty.");
  }

  uint64_t total_weight = 0;
  for (uint32_t w : weights) {
    total_weight += w;
  }
  if (total_weight == 0 || total_weight > UINT32_MAX) {
    return core::InvalidArgumentError(
        "Total weight must be between 1 and UINT32_MAX.");
  }

  const uint32_t r = Rand<uint32_t>(weights.size());
  const uint32_t target = ScaleToRange(r, static_cast<uint32_t>(total_weight));

  uint64_t cumulative = 0;
  for (size_t i = 0; i < weights.size(); ++i) {
    cumulative += weights[i];
    if (target < cumulative) {
      return i;
    }
  }

  return weights.size() - 1;
}

}  // namespace core
