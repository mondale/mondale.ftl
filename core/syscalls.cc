#include <unistd.h>

#include <concepts>

#include "core/strings.h"
#include "core/syscalls.h"

namespace core::syscalls {
namespace {

// Syscall is a lambda returning Ret, nominally an int.
// Acceptance function is a lambda taking Ret and returning bool; return true
// when the syscall has succeeded.
template <typename Ret = int, typename Syscall, typename AcceptanceFn>
  requires std::invocable<Syscall> &&
           std::same_as<std::invoke_result_t<Syscall>, Ret> &&
           std::predicate<AcceptanceFn, Ret>
ResultOr<Ret> SyscallRetryEintr(Syscall&& s, AcceptanceFn&& acc) {
  int saved_errno = 0;
  do {
    const Ret ret = s();
    saved_errno = errno;
    if (acc(ret)) return ret;
  } while (saved_errno == EINTR);

  // Error case.
  return ResultFromErrno(saved_errno);
}

Result NoReturnNonZero(int ret, std::string_view syscall) {
  if (ret == 0) {
    return Result::Ok();
  }
  return Result(
      Code::kError,
      strings::Format("Successful {} should not also return 0.", syscall));
}

}  // namespace

Result Access(std::string_view path, int mode) {
  auto syscall = [&]() -> int { return ::access(path.data(), mode); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr(syscall, accept));
  TRY(NoReturnNonZero(ret, "Accept"));
  return Result::Ok();
}

ResultOr<struct stat> Stat(std::string_view path) {
  struct stat sb;
  auto syscall = [&]() -> int { return ::stat(path.data(), &sb); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr(syscall, accept));
  TRY(NoReturnNonZero(ret, "Stat"));
  return sb;
}

ResultOr<struct stat> FStat(const FileDescriptor& fd) {
  struct stat sb;
  auto syscall = [&]() -> int { return ::fstat(fd.fd(), &sb); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr(syscall, accept));
  TRY(NoReturnNonZero(ret, "Ftat"));
  return sb;
}

ResultOr<FileDescriptor> Open(std::string_view path, int flags, mode_t mode) {
  auto syscall = [&]() -> int { return ::open(path.data(), flags, mode); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int raw_fd, SyscallRetryEintr(syscall, accept));
  return FileDescriptor(raw_fd);
}

ResultOr<size_t> Read(const FileDescriptor& fd, char* buf, size_t count) {
  auto syscall = [&]() -> ssize_t { return ::read(fd.fd(), buf, count); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const ssize_t bytes, SyscallRetryEintr<ssize_t>(syscall, accept));
  return static_cast<size_t>(bytes);
}

ResultOr<size_t> Write(const FileDescriptor& fd, const char* buf,
                       size_t count) {
  auto syscall = [&]() -> ssize_t { return ::write(fd.fd(), buf, count); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const ssize_t bytes, SyscallRetryEintr<ssize_t>(syscall, accept));
  return static_cast<size_t>(bytes);
}

}  // namespace core::syscalls
