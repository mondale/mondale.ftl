#ifndef NET_LISTENER_H_
#define NET_LISTENER_H_

#include <functional>

#include "core/vocabulary.h"
#include "io/io.h"
#include "net/socket_address.h"

namespace net {

class Listener final : public io::IoHandler {
 private:
  struct PrivateTag {};

 public:
  using AcceptFn =
      std::move_only_function<void(core::FileDescriptor, net::SocketAddress)>;
  Listener(PrivateTag, AcceptFn af);

  static ResultOr<std::shared_ptr<Listener>> Build(io::Epoller* ep,
                                                   uint16_t port, AcceptFn af);

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;
  Result HandleIdle(io::FdHandle h, const core::FileDescriptor& fd) override;

 private:
  AcceptFn on_accept_;
};

}  // namespace net

#endif  // #ifndef NET_LISTENER_H_
