#include "core/syscalls.h"
#include "http/listener.h"

namespace http {
namespace {

Result NonBlockingAccept(
    io::IoHandler::Outcome* o, const core::FileDescriptor& fd,
    const std::move_only_function<void(core::FileDescriptor)>& fn) {
  auto maybe_fd = core::syscalls::Accept4(fd, nullptr, nullptr,
                                          SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (maybe_fd.IsOk()) {
    *o = io::IoHandler::Outcome::kYield;
    // extract fd TODO
    return Result::Ok();
  } else if (maybe_fd.result().Is(Code::kEagain)) {
    *o = io::IoHandler::Outcome::kFdEagain;
    return Result::Ok();
  }
  return maybe_fd.result();
}

}  // namespace

Listener::Listener(
    std::move_only_function<void(core::FileDescriptor)> on_accept)
    : on_accept_(std::move(on_accept)) {
  set_idle_threshold(base::Days(1));
}

ResultOr<io::IoHandler::Outcome> Listener::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kSuspend;
  TRY(NonBlockingAccept(&o, fd, on_accept_));
  return o;
}

ResultOr<io::IoHandler::Outcome> Listener::HandleWrite(
    io::FdHandle h, const core::FileDescriptor& fd) {
  // Never write to a listen socket.
  return Outcome::kSuspend;
}

Result Listener::HandleIdle(io::FdHandle h, const core::FileDescriptor& fd) {
  // A listen socket is never late, Frodo Baggings. Nor is it early. It becomes
  // readable precisely when it means to.
  return Result::Ok();
}

}  // namespace http
