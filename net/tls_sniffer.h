#ifndef NET_TLS_SNIFFER_H_
#define NET_TLS_SNIFFER_H_

#include "core/vocabulary.h"
#include "io/io.h"
#include "net/socket_address.h"

namespace net {

// Peeks at the first two bytes on a socket and decides if it's a TLS or non-TLS
// socket. False positives possible so... yea.
class TlsSniffer final : public io::IoHandler {
 protected:
  struct PrivateTag {};

 public:
  TlsSniffer(PrivateTag, net::SocketAddress sa,
             std::move_only_function<void(bool, core::FileDescriptor,
                                          net::SocketAddress)>
                 is_tls)
      : sa_(sa), is_tls_(std::move(is_tls)) {}

  static std::shared_ptr<TlsSniffer> New(
      net::SocketAddress sa,
      std::move_only_function<void(bool, core::FileDescriptor,
                                   net::SocketAddress)>
          is_tls) {
    return std::make_shared<TlsSniffer>(PrivateTag{}, sa, std::move(is_tls));
  }

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;
  void HandleEject(core::FileDescriptor fd) override;

 private:
  net::SocketAddress sa_;
  std::move_only_function<void(bool, core::FileDescriptor, net::SocketAddress)>
      is_tls_;
  bool probably_tls_ = false;
};

}  // namespace net

#endif  // #ifndef NET_TLS_SNIFFER_H_
