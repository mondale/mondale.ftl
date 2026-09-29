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

  void Post(io::FdHandle h, io::SourceBuffer sb);
  void Post(io::FdHandle h, io::SinkBuffer sb);

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;

 private:
  ResultOr<Outcome> HandleReadEstablished(io::FdHandle h,
                                          const core::FileDescriptor& fd);
  ResultOr<Outcome> HandleWriteEstablished(io::FdHandle h,
                                           const core::FileDescriptor& fd);
  ResultOr<Outcome> AttemptRead(io::FdHandle h, const core::FileDescriptor& fd);
  ResultOr<Outcome> AttemptWrite(io::FdHandle h,
                                 const core::FileDescriptor& fd);
  void* const ssl_;
  bool read_wants_write_ = false;
  bool write_wants_read_ = false;
  SslHandshaker handshaker_;
  std::queue<io::SourceBuffer> sources_;
  std::queue<io::SinkBuffer> sinks_;
};

}  // namespace net

#endif  // #ifndef NET_SSL_SOCKET_H_
