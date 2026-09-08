#include "capsule/storage.h"

namespace capsule {
namespace {

void* Advance(void* p, size_t n) {
  return reinterpret_cast<void*>(reinterpret_cast<char*>(p) + n);
}

}  // namespace

Storage::Storage(std::shared_ptr<StorageFactory::Alloc> a)
    : base_(a->data()), n_(a->n()), alloc_(std::move(a)) {}

Storage::Storage(std::shared_ptr<StorageFactory::Alloc> a, void* d, size_t n)
    : base_(d), n_(n), alloc_(std::move(a)) {}

// static
ResultOr<std::unique_ptr<Storage>> Storage::Allocate(
    std::shared_ptr<StorageFactory> f, size_t n) {
  TRY_ASSIGN(auto a, f->NewAlloc(n));
  return std::make_unique<Storage>(std::move(a));
}

ResultOr<std::unique_ptr<Storage>> Storage::Carve(size_t trim_head,
                                                  size_t trim_tail) const {
  if (trim_head + trim_tail >= n()) {
    return core::CapsuleFatalError(strings::Format(
        "Trim of [{}] from head and [{}] from tail of storage of length [{}].",
        trim_head, trim_tail, n()));
  }
  const auto s = n() - trim_head - trim_tail;
  auto* const b = Advance(base_, trim_head);
  return std::make_unique<Storage>(alloc_, b, s);
}

std::unique_ptr<Storage> Storage::Ref() const {
  return std::make_unique<Storage>(alloc_, base(), n());
}

}  // namespace capsule
