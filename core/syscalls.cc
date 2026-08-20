#include <concepts>

#include "core/syscalls.h"

namespace core::syscalls {
namespace {

// Syscall is a lambda returning int.
// Acceptance function is a lambda taking int and returning bool; return true
// when the syscall has succeeded.
template <typename Syscall, typename AcceptanceFn>
  requires std::invocable<Syscall> &&
           std::same_as<std::invoke_result_t<Syscall>, int> &&
           std::predicate<AcceptanceFn, int>
ResultOr<int> SyscallRetryEintr(Syscall&& s, AcceptanceFn&& acc) {
  int saved_errno = 0;
  do {
    const int ret = s();
    saved_errno = errno;
    if (acc(ret)) return ret;
  } while (saved_errno == EINTR);

  // Error case.
  return ResultFromErrno(saved_errno);
}

}  // namespace

ResultOr<FileDescriptor> Open(std::string_view path, int flags, mode_t mode) {
  auto syscall = [&]() -> int { return ::open(path.data(), flags, mode); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int raw_fd, SyscallRetryEintr(syscall, accept));
  return FileDescriptor(raw_fd);
}

}  // namespace core::syscalls
