#ifndef HTTP_WEB_HANDLER_H_
#define HTTP_WEB_HANDLER_H_

#include <vector>

#include "core/vocabulary.h"
#include "io/io.h"

namespace http {

class WebHandler final : public io::DataHandler {
 private:
  struct PrivateTag {};

 public:
  explicit WebHandler(PrivateTag);
  WebHandler() = delete;

  static std::shared_ptr<WebHandler> New();

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override;

 private:
  Result TryParse();

  std::vector<char> buf_;
  size_t bytes_ = 0;
};

}  // namespace http

#endif  // #ifndef HTTP_WEB_HANDLER_H_
