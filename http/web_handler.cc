#include "base/flags.h"
#include "http/web_handler.h"

FLAG_COHORT(http);
FLAG(int, initial_buffer_bytes, 8192).Ge(8).Lt(65536);

namespace http {

std::shared_ptr<WebHandler> WebHandler::New() {
  return std::make_shared<WebHandler>(PrivateTag{});
}

WebHandler::WebHandler(PrivateTag)
    : io::DataHandler(DataHandler::PrivateTag{}) {
  buf_.resize(FLAG_LOOKUP(initial_buffer_bytes));
}

ResultOr<io::IoHandler::Outcome> WebHandler::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kSuspend;
  const auto remain = buf_.size() - bytes_;
  TRY_ASSIGN(const auto n,
             NonBlockingRead(&o, fd, buf_.data() + bytes_, remain));
  bytes_ += n;
  if (n > 0) {
    TRY(TryParse());
  }
  return o;
}

Result WebHandler::TryParse() { return Result::Ok(); }

}  // namespace http
