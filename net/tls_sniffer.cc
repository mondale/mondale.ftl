#include "net/tls_sniffer.h"

namespace net {

ResultOr<io::IoHandler::Outcome> TlsSniffer::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  Outcome ret = Outcome::kSuspend;
  char buf[2];
  TRY_ASSIGN(const auto bytes, NonBlockingRecv(&ret, fd, buf, 2, MSG_PEEK));
  if (1 == bytes) {
    return core::StreamFatalError(strings::Format(
        "Inbound connection from [{}] sent a single byte.", sa_.ToString()));
  }

  // We can classify if we have two bytes.
  if (2 == bytes) {
    if (buf[0] == 0x16 && buf[1] == 0x03) {
      // These are the first two bytes of TLS.
      probably_tls_ = true;
    } else {
      // HTTP sessions begin with one of these:
      // GE, PO, HE, DE, OP, TR, CO, PU, PA
      bool is_http = false;
      is_http = is_http || (buf[0] == 'G' && buf[1] == 'E');
      is_http = is_http || (buf[0] == 'P' && buf[1] == 'O');
      is_http = is_http || (buf[0] == 'H' && buf[1] == 'E');
      is_http = is_http || (buf[0] == 'D' && buf[1] == 'E');
      is_http = is_http || (buf[0] == 'O' && buf[1] == 'P');
      is_http = is_http || (buf[0] == 'T' && buf[1] == 'R');
      is_http = is_http || (buf[0] == 'C' && buf[1] == 'O');
      is_http = is_http || (buf[0] == 'P' && buf[1] == 'U');
      is_http = is_http || (buf[0] == 'P' && buf[1] == 'A');
      is_http = is_http || (buf[0] == 'G' && buf[1] == 'E');
      if (!is_http) {
        auto ret = core::StreamFatalError(
            strings::Format("Inbound connection from [{}] classifies as "
                            "neither HTTP nor HTTPS.",
                            sa_.ToString()));
        Log(WARNING) << ret;
        return ret;
      }
      probably_tls_ = false;
    }
    io::Context::Current()->Eject(h);
    return Outcome::kSuspend;
  }
  return ret;
}

ResultOr<io::IoHandler::Outcome> TlsSniffer::HandleWrite(
    io::FdHandle h, const core::FileDescriptor& fd) {
  return Outcome::kSuspend;
}

void TlsSniffer::HandleEject(core::FileDescriptor fd) {
  is_tls_(probably_tls_, std::move(fd), sa_);
}

}  // namespace net
