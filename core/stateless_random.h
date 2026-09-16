#ifndef CORE_STATELESS_RANDOM_H_
#define CORE_STATELESS_RANDOM_H_

#include <cstdint>
#include <span>

#include "core/vocabulary.h"

namespace core {

uint64_t Rand64(uint64_t entropy = 0);

template <typename T>
T Rand(uint64_t entropy = 0) {
  return static_cast<T>(Rand64(entropy));
}

ResultOr<size_t> WeightedSelect(std::span<const uint32_t> weights);

}  // namespace core

#endif  // #ifndef CORE_STATELESS_RANDOM_H_
