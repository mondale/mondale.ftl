#include "core/idioms.h"
#include "core/syscalls.h"
#include "net/listener.h"
#include "net/socket_address.h"

namespace net {
namespace {

Result NonBlockingAccept(io::IoHandler::Outcome* o,
                         const core::FileDescriptor& fd,
                         Listener::AcceptFn& fn) {
  SocketAddress sa;
  socklen_t unused = 0;
  auto maybe_fd = core::syscalls::Accept4(fd, sa.AsSockaddr(), &unused,
                                          SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (maybe_fd.IsOk()) {
    *o = io::IoHandler::Outcome::kYield;
    auto fd = std::move(maybe_fd.ValueOrDie());
    fn(std::move(fd), sa);
    return Result::Ok();
  } else if (maybe_fd.result().Is(Code::kEagain)) {
    *o = io::IoHandler::Outcome::kFdEagain;
    return Result::Ok();
  }
  return maybe_fd.result();
}

}  // namespace

//  static
ResultOr<std::shared_ptr<Listener>> Listener::Build(io::Epoller* ep,
                                                    uint16_t port,
                                                    AcceptFn af) {
  constexpr int kBacklog = 100;
  TRY_ASSIGN(auto fd, core::idioms::NewListenSocket(port, kBacklog));
  auto l = std::make_shared<Listener>(PrivateTag{}, std::move(af));
  TRY(ep->Register(std::move(l), std::move(fd)));
  return l;
}

Listener::Listener(PrivateTag, AcceptFn af) : on_accept_(std::move(af)) {
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

}  // namespace net
