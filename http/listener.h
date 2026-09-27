#ifndef HTTP_LISTENER_H_
#define HTTP_LISTENER_H_

#include <functional>

#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace http {

class Listener final : public io::IoHandler {
 public:
  explicit Listener(
      std::move_only_function<void(core::FileDescriptor)> on_accept);

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override;
  Result HandleIdle(io::FdHandle h, const core::FileDescriptor& fd) override;

 private:
  std::move_only_function<void(core::FileDescriptor)> on_accept_;
};

}  // namespace http

#endif  // #ifndef HTTP_LISTENER_H_
