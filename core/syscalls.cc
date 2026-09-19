#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
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
  TRY(NoReturnNonZero(ret, "FStat"));
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

ResultOr<struct rlimit> GetRLimit(int resource) {
  struct rlimit lim;
  auto syscall = [&]() -> int { return ::getrlimit(resource, &lim); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  TRY(NoReturnNonZero(ret, "GetRLimit"));
  return lim;
}

Result SetRLimit(int resource, const struct rlimit* l) {
  auto syscall = [&]() -> int { return ::setrlimit(resource, l); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return NoReturnNonZero(ret, "SetRLimit");
}

Result Madvise(void* p, size_t n, int advice) {
  auto syscall = [&]() -> int { return ::madvise(p, n, advice); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return NoReturnNonZero(ret, "Madvise");
}

ResultOr<FileDescriptor> EpollCreate1(int flags) {
  auto syscall = [&]() -> int { return ::epoll_create1(flags); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int raw_fd, SyscallRetryEintr(syscall, accept));
  return FileDescriptor(raw_fd);
}

Result EpollCtl(const FileDescriptor& epfd, int op, const FileDescriptor& fd,
                struct epoll_event* event) {
  auto syscall = [&]() -> int {
    return ::epoll_ctl(epfd.fd(), op, fd.fd(), event);
  };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return NoReturnNonZero(ret, "EpollCtl");
}

ResultOr<FileDescriptor> EventFd(unsigned int initval, int flags) {
  auto syscall = [&]() -> int { return ::eventfd(initval, flags); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int raw_fd, SyscallRetryEintr(syscall, accept));
  return FileDescriptor(raw_fd);
}

ResultOr<uint64_t> EventFdRead(const FileDescriptor& fd) {
  uint64_t value = 0;
  auto syscall = [&]() -> int { return ::eventfd_read(fd.fd(), &value); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  TRY(NoReturnNonZero(ret, "EventFdRead"));
  return value;
}

Result EventFdWrite(const FileDescriptor& fd, eventfd_t value) {
  auto syscall = [&]() -> int { return ::eventfd_write(fd.fd(), value); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return NoReturnNonZero(ret, "EventFdWrite");
}

ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd) {
  auto syscall = [&]() -> int { return ::fcntl(fd.fd(), cmd); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return ret;
}

ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd, int arg) {
  auto syscall = [&]() -> int { return ::fcntl(fd.fd(), cmd, arg); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return ret;
}

ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd, void* arg) {
  auto syscall = [&]() -> int { return ::fcntl(fd.fd(), cmd, arg); };
  auto accept = [](int ret) -> bool { return ret >= 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  return ret;
}

ResultOr<int> EpollPwait2(const FileDescriptor& epfd,
                          struct epoll_event* events, int maxevents,
                          const struct timespec* timeout,
                          const sigset_t* sigmask) {
  const int ret =
      ::epoll_pwait2(epfd.fd(), events, maxevents, timeout, sigmask);
  if (ret < 0 && errno == EINTR) {
    return 0;
  } else if (ret < 0) {
    return ResultFromErrno(errno);
  }
  return ret;
}

ResultOr<std::pair<FileDescriptor, FileDescriptor>> Pipe2(int flags) {
  int fds[2];
  auto syscall = [&]() -> int { return ::pipe2(fds, flags); };
  auto accept = [](int ret) -> bool { return ret == 0; };
  TRY_ASSIGN(const int ret, SyscallRetryEintr<int>(syscall, accept));
  TRY(NoReturnNonZero(ret, "Pipe2"));
  return std::make_pair(FileDescriptor(fds[0]), FileDescriptor(fds[1]));
}

}  // namespace core::syscalls
