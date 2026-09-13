#include <sys/mman.h>

#include "core/handle_table.h"
#include "core/syscalls.h"

namespace core {
namespace internal {

HandleTableBase::HandleTableBase(int max_n)
    : n_(max_n),
      index_bits_(std::bit_width(static_cast<unsigned>(max_n - 1))) {}

int64_t HandleTableBase::AllocateHandle() {
  int index = 0;
  if (!free_indexes_.empty()) {
    index = free_indexes_.back();
    free_indexes_.pop_back();
  } else {
    index = next_allocate_++;
  }

  int64_t h = 0;
  do {
    h = (generation_++ << index_bits_) | index;
  } while (h == 0);

  return h;
}

void HandleTableBase::MaybeMadviseStorage(void* addr, size_t size) {
  constexpr size_t kThreshold = 2 * 1024 * 1024;  // 2 MiB
  if (size <= kThreshold) {
    return;
  }
  void* target = reinterpret_cast<char*>(addr) + kThreshold;
  size_t len = size - kThreshold;
  Result result = syscalls::Madvise(target, len, MADV_DONTNEED);
  Log(WARNING, If(!result.IsOk())) << "Madvise failed" << result;
}

}  // namespace internal
}  // namespace core
