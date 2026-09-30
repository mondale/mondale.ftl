#ifndef NET_SSL_SOCKET_H_
#define NET_SSL_SOCKET_H_

#include "core/vocabulary.h"
#include "io/io.h"
#include "net/socket_address.h"
#include "net/ssl_context.h"
#include "net/ssl_handshaker.h"

namespace net {

class SslSocket final : public io::IoHandler {
 private:
  struct PrivateTag {};

 public:
  SslSocket(PrivateTag, void* ssl, SslHandshaker shs);
  virtual ~SslSocket();

  static ResultOr<std::shared_ptr<SslSocket>> New(io::Epoller* e,
                                                  net::SslContext* sc,
                                                  core::FileDescriptor fd,
                                                  net::SocketAddress sa,
                                                  SslHandshaker shs);

  void Post(io::SourceBuffer sb);
  void Post(io::SinkBuffer sb);
  void PostClose();

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;

 private:
  ResultOr<Outcome> HandleReadEstablished();
  ResultOr<Outcome> HandleWriteEstablished();
  ResultOr<Outcome> AttemptRead();
  ResultOr<Outcome> AttemptWrite();

  void* const ssl_;
  io::FdHandle h_ = io::FdHandle::kInvalid;
  bool read_wants_write_ = false;
  bool write_wants_read_ = false;
  bool close_requested_ = false;
  SslHandshaker handshaker_;
  std::queue<io::SourceBuffer> sources_;
  std::queue<io::SinkBuffer> sinks_;
};

}  // namespace net

#endif  // #ifndef NET_SSL_SOCKET_H_
