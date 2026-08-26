#include <numa.h>
#include <numaif.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstddef>

#include "base/numa.h"

namespace base {
namespace internal {

void* AllocateNuma(std::size_t sz, int node) {
  return numa_alloc_onnode(sz, node);
}

void FreeNuma(void* ptr, std::size_t sz) {
  if (!ptr) return;
  numa_free(ptr, sz);
}

}  // namespace internal

int GetAddressNumaNode(const void* ptr) {
  int mode = -1;
  // MPOL_F_NODE | MPOL_F_ADDR tells get_mempolicy to return the node ID for the
  // given address.
  long res = syscall(SYS_get_mempolicy, &mode, nullptr, 0,
                     const_cast<void*>(ptr), MPOL_F_NODE | MPOL_F_ADDR);
  if (res < 0) {
    return -1;
  }
  return mode;
}

}  // namespace base
