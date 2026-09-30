#ifndef HTTP_WEB_SOCKET_H_
#define HTTP_WEB_SOCKET_H_

#include <vector>

#include "core/vocabulary.h"
#include "io/io.h"
#include "net/socket_address.h"

namespace http {

class WebSocket final : public io::IoHandler, public io::Poster {
 private:
  struct PrivateTag {};

 public:
  explicit WebSocket(PrivateTag);
  WebSocket() = delete;

  static ResultOr<std::shared_ptr<WebSocket>> New(io::Epoller* e,
                                                  core::FileDescriptor fd,
                                                  net::SocketAddress sa);

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;

  void Post(io::SourceBuffer sb) override;
  void Post(io::SinkBuffer sb) override;
  void PostClose() override;

 private:
  io::FdHandle h_ = io::FdHandle::kInvalid;
  bool close_requested_;
  std::queue<io::SourceBuffer> sources_;
  std::queue<io::SinkBuffer> sinks_;
};

}  // namespace http

#endif  // #ifndef HTTP_WEB_SOCKET_H_
