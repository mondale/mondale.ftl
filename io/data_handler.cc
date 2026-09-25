#include "io/context.h"
#include "io/data_handler.h"

namespace io {

ResultOr<IoHandler::Outcome> DataHandler::HandleRead(
    FdHandle h, const core::FileDescriptor& fd) {
  return Outcome::kSuspend;
}

ResultOr<IoHandler::Outcome> DataHandler::HandleWrite(
    FdHandle h, const core::FileDescriptor& fd) {
  Outcome ret = Outcome::kSuspend;
  auto& p = perfd_[h];
  if (p.sources.empty()) return ret;
  auto& buf = p.sources.front();
  TRY_ASSIGN(const auto bytes,
             NonBlockingWrite(&ret, fd, buf.data(), buf.size()));
  if (buf.Consume(bytes)) {
    p.sources.pop();
  }
  return ret;
}

void DataHandler::Post(FdHandle h, SourceBuffer sb) {
  auto& p = perfd_[h];
  if (p.sources.empty()) Context::Current()->RequestWrite(h);
  p.sources.emplace(std::move(sb));
}

}  // namespace io
