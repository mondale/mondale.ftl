#include "io/context.h"
#include "io/data_handler.h"

namespace io {

ResultOr<FdHandle> DataHandler::Add(core::FileDescriptor fd) {
  auto self = shared_from_this();
  return Context::Current()->Add(std::move(self), std::move(fd));
}

ResultOr<IoHandler::Outcome> DataHandler::HandleRead(
    FdHandle h, const core::FileDescriptor& fd) {
  Outcome ret = Outcome::kSuspend;
  auto& p = perfd_[h];
  if (p.sources.empty()) return ret;
  auto& buf = p.sinks.front();
  TRY_ASSIGN(const auto bytes,
             NonBlockingRead(&ret, fd, buf.data(), buf.size()));
  if (buf.Advance(bytes)) {
    p.sinks.pop();
  }
  return ret;
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

void DataHandler::Post(FdHandle h, SinkBuffer sb) {
  auto& p = perfd_[h];
  if (p.sinks.empty()) Context::Current()->RequestRead(h);
  p.sinks.emplace(std::move(sb));
}

}  // namespace io
