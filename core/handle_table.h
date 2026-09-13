#ifndef CORE_HANDLE_TABLE_H_
#define CORE_HANDLE_TABLE_H_

#include <bit>
#include <cstddef>
#include <memory>
#include <vector>

#include "core/handle.h"
#include "core/vocabulary.h"

namespace core {
namespace internal {

class HandleTableBase {
 protected:
  explicit HandleTableBase(int max_n);
  ~HandleTableBase() = default;

  bool CanAllocate() const {
    return !free_indexes_.empty() || next_allocate_ < n_;
  }

  int GetIndex(int64_t h) const {
    int64_t mask = (1LL << index_bits_) - 1;
    return static_cast<int>(h & mask);
  }

  bool ValidateHandle(int64_t h) const {
    int index = GetIndex(h);
    return index >= 0 && index < n_;
  }

  int64_t AllocateHandle();

  void FreeHandle(int64_t h) { free_indexes_.push_back(GetIndex(h)); }

  void MaybeMadviseStorage(void* addr, size_t size);

  const int n_;
  const int index_bits_;
  int next_allocate_{0};
  int64_t generation_{1};
  std::vector<int> free_indexes_;
};

}  // namespace internal

template <typename T, int N>
class HandleTable final : public internal::HandleTableBase {
 public:
  HANDLE_TYPE(Handle, int64_t);

  HandleTable() : internal::HandleTableBase(N) {
    storage_ = std::make_unique<Slot[]>(N);
    MaybeMadviseStorage(storage_.get(), sizeof(Slot) * N);
  }

  ~HandleTable() {
    for (int i = 0; i < next_allocate_; ++i) {
      if (storage_[i].handle != 0) {
        std::destroy_at(reinterpret_cast<T*>(storage_[i].storage));
      }
    }
  }

  HandleTable(const HandleTable&) = delete;
  HandleTable& operator=(const HandleTable&) = delete;

  ResultOr<Handle> Allocate() {
    if (!CanAllocate()) {
      return Result(Code::kExhausted);
    }

    int64_t raw_h = AllocateHandle();
    int index = GetIndex(raw_h);

    Slot& slot = storage_[index];
    slot.handle = raw_h;
    std::construct_at(reinterpret_cast<T*>(slot.storage));

    return Handle(raw_h);
  }

  ResultOr<T*> Lookup(Handle h) {
    int64_t raw_h = h.value();
    if (!ValidateHandle(raw_h)) {
      return Result(Code::kInvalidArgument);
    }

    int index = GetIndex(raw_h);
    Slot& slot = storage_[index];
    if (slot.handle != raw_h || slot.handle == 0) {
      return Result(Code::kInvalidArgument);
    }

    return reinterpret_cast<T*>(slot.storage);
  }

  Result Free(Handle h) {
    int64_t raw_h = h.value();
    if (!ValidateHandle(raw_h)) {
      return Result(Code::kInvalidArgument);
    }

    int index = GetIndex(raw_h);
    Slot& slot = storage_[index];
    if (slot.handle != raw_h || slot.handle == 0) {
      return Result(Code::kInvalidArgument);
    }

    std::destroy_at(reinterpret_cast<T*>(slot.storage));
    slot.handle = 0;
    FreeHandle(raw_h);

    return Result::Ok();
  }

 private:
  struct alignas(64) Slot {
    alignas(T) std::byte storage[sizeof(T)];
    int64_t handle{0};
  };

  std::unique_ptr<Slot[]> storage_;
};

}  // namespace core

#endif  // #ifndef CORE_HANDLE_TABLE_H_
