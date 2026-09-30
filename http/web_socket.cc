#include "http/web_socket.h"

namespace http {

ResultOr<std::shared_ptr<WebSocket>> WebSocket::New(io::Epoller* e,
                                                    core::FileDescriptor fd,
                                                    net::SocketAddress sa) {
  static_cast<void>(sa);  // idk what to do with this yet.
  auto ret = std::make_shared<WebSocket>(PrivateTag{});
  TRY(e->Register(ret, std::move(fd)));
  return ret;
}

WebSocket::WebSocket(PrivateTag) {}

ResultOr<io::IoHandler::Outcome> WebSocket::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  h_ = h;
  Outcome o = Outcome::kSuspend;
  if (sinks_.empty()) return o;

  auto& s = sinks_.front();
  TRY_ASSIGN(const auto n, NonBlockingRead(&o, fd, s.data(), s.size()));
  if (n > 0) {
    static_cast<void>(s.Advance(n));
    sinks_.pop();  // Admit partial fills of the buffer.
  }
  return o;
}

ResultOr<io::IoHandler::Outcome> WebSocket::HandleWrite(
    io::FdHandle h, const core::FileDescriptor& fd) {
  h_ = h;
  Outcome o = Outcome::kSuspend;
  if (sources_.empty()) return o;

  auto& s = sources_.front();
  TRY_ASSIGN(const auto n, NonBlockingWrite(&o, fd, s.data(), s.size()));
  if (s.Consume(n)) {
    sources_.pop();
  }

  if (sources_.empty() && close_requested_) {
    o = Outcome::kClose;
  }
  return o;
}

void WebSocket::Post(io::SourceBuffer sb) {
  if (close_requested_) return;
  if (sources_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestWrite(h_);
  }
  sources_.push(std::move(sb));
}

void WebSocket::Post(io::SinkBuffer sb) {
  if (sinks_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestRead(h_);
  }
  sinks_.push(std::move(sb));
}

void WebSocket::PostClose() {
  close_requested_ = true;
  if (sources_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestWrite(h_);
  }
}

}  // namespace http
