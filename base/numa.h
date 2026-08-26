#ifndef BASE_NUMA_H_
#define BASE_NUMA_H_

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

#include "base/cpu.h"

namespace base {
namespace internal {

void* AllocateNuma(std::size_t sz, int node);
void FreeNuma(void* ptr, std::size_t sz);

template <typename T>
struct NumaDeleter {
  std::size_t size = sizeof(T);

  void operator()(T* ptr) const {
    if (ptr) {
      std::destroy_at(ptr);
      FreeNuma(static_cast<void*>(ptr), size);
    }
  }
};

}  // namespace internal

template <typename T>
using NumaUniquePtr = std::unique_ptr<T, internal::NumaDeleter<T>>;

// Allocates on a specific NUMA node
template <typename T, typename... Args>
  requires(!std::is_array_v<T>)
NumaUniquePtr<T> MakeUniqueNumaAwareOnNode(int nd, Args&&... args) {
  void* mem = internal::AllocateNuma(sizeof(T), nd);
  if (!mem) {
    return nullptr;
  }

  T* ptr = ::new (mem) T(std::forward<Args>(args)...);
  return NumaUniquePtr<T>(ptr, internal::NumaDeleter<T>{.size = sizeof(T)});
}

// Allocates on the caller's current NUMA domain
template <typename T, typename... Args>
  requires(!std::is_array_v<T>)
NumaUniquePtr<T> MakeUniqueNumaAware(Args&&... args) {
  int cur = CurrentCpu();
  int nd = NumaDomainFor(cur);
  if (nd < 0) {
    nd = 0;
  }
  return MakeUniqueNumaAwareOnNode<T>(nd, std::forward<Args>(args)...);
}

// Returns the NUMA node with affinity for 'ptr', or -1 if for some goddamn
// reason the node can't be determined. This has a syscall inside so it ain't
// snappy.
int GetAddressNumaNode(const void* ptr);

}  // namespace base

#endif  // #ifndef BASE_NUMA_H_
