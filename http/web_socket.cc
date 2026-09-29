#include "base/flags.h"
#include "http/web_socket.h"

FLAG_COHORT(http);
FLAG(int, initial_buffer_bytes, 8192).Ge(8).Lt(65536);

namespace http {

ResultOr<std::shared_ptr<WebSocket>> WebSocket::New(io::Epoller* e,
                                                    core::FileDescriptor fd,
                                                    net::SocketAddress sa) {
  static_cast<void>(sa);  // idk what to do with this yet.
  auto ret = std::make_shared<WebSocket>(PrivateTag{});
  TRY(e->Register(ret, std::move(fd)));
  return ret;
}

WebSocket::WebSocket(PrivateTag) {
  // buf_.resize(FLAG_LOOKUP(initial_buffer_bytes));
}

ResultOr<io::IoHandler::Outcome> WebSocket::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kSuspend;
  /*
    const auto remain = buf_.size() - bytes_;
    TRY_ASSIGN(const auto n,
               NonBlockingRead(&o, fd, buf_.data() + bytes_, remain));
  */
  return o;
}

ResultOr<io::IoHandler::Outcome> WebSocket::HandleWrite(
    io::FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kSuspend;
  return o;
}

}  // namespace http
