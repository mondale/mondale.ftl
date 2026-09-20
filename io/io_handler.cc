#include "core/syscalls.h"
#include "io/io_handler.h"

namespace io {

ResultOr<size_t> IoHandler::NonBlockingRead(IoHandler::Outcome* outcome,
                                            const core::FileDescriptor& fd,
                                            char* buf, size_t count) const {
  auto r = core::syscalls::Read(fd, buf, count);
  if (r.IsOk()) {
    const auto bytes_read = r.ValueOrDie();
    if (0 == bytes_read) {
      // EOF -> close the FD.
      *outcome = IoHandler::Outcome::kClose;
      return 0;
    }

    // Could be a full or partial read, either way, yield and call again.
    *outcome = IoHandler::Outcome::kYield;
    return bytes_read;
  } else if (r.result().Is(Code::kEagain)) {
    // No more bytes, convert EAGAIN to an ok status.
    *outcome = IoHandler::Outcome::kFdEagain;
    return 0;
  }

  *outcome = IoHandler::Outcome::kClose;
  return r.result();
}

ResultOr<size_t> IoHandler::NonBlockingWrite(IoHandler::Outcome* outcome,
                                             const core::FileDescriptor& fd,
                                             const char* buf,
                                             size_t count) const {
  auto r = core::syscalls::Write(fd, buf, count);
  if (r.IsOk()) {
    // Presumably more bytes are writeable, so call again.
    *outcome = IoHandler::Outcome::kYield;
    return r.ValueOrDie();
  } else if (r.result().Is(Code::kEagain)) {
    // Backpressure, convert EAGAIN to an ok status.
    *outcome = IoHandler::Outcome::kFdEagain;
    return 0;
  }

  *outcome = IoHandler::Outcome::kClose;
  return r.result();
}

std::string ToString(IoHandler::Outcome o) {
  switch (o) {
    case IoHandler::Outcome::kFdEagain:
      return "kFdEagain";
    case IoHandler::Outcome::kYield:
      return "kYield";
    case IoHandler::Outcome::kSuspend:
      return "kSuspend";
    case IoHandler::Outcome::kClose:
      return "kClose";
  }
}

std::ostream& operator<<(std::ostream& out, IoHandler::Outcome o) {
  out << ToString(o);
  return out;
}

}  // namespace io
