#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "net/ssl_socket.h"

namespace net {
namespace {

ResultOr<size_t> NonBlockingSslRead(io::IoHandler::Outcome* o, io::FdHandle h,
                                    SSL* ssl, bool* weird, char* buf,
                                    size_t count) {
  const int ret = SSL_read(ssl, buf, count);
  if (ret > 0) {
    *o = io::IoHandler::Outcome::kYield;
    return static_cast<size_t>(ret);
  }
  const int err = SSL_get_error(ssl, ret);
  if (err == SSL_ERROR_WANT_READ) {
    *o = io::IoHandler::Outcome::kFdEagain;
    return 0;
  }
  if (err == SSL_ERROR_WANT_WRITE) {
    // Weird case...
    *weird = true;
    *o = io::IoHandler::Outcome::kSuspend;
    io::Context::Current()->RequestWrite(h);
    return 0;
  }
  return core::StreamFatalError(
      strings::Format("Unhandled SSL error during read [{}]", err));
}

ResultOr<size_t> NonBlockingSslWrite(io::IoHandler::Outcome* o, io::FdHandle h,
                                     SSL* ssl, bool* weird, const char* buf,
                                     size_t count) {
  const int ret = SSL_write(ssl, buf, count);
  if (ret > 0) {
    *o = io::IoHandler::Outcome::kYield;
    return static_cast<size_t>(ret);
  }
  const int err = SSL_get_error(ssl, ret);
  if (err == SSL_ERROR_WANT_READ) {
    // Weird case...
    *weird = true;
    *o = io::IoHandler::Outcome::kSuspend;
    io::Context::Current()->RequestRead(h);
    return 0;
  }
  if (err == SSL_ERROR_WANT_WRITE) {
    *o = io::IoHandler::Outcome::kFdEagain;
    return 0;
  }
  return core::StreamFatalError(
      strings::Format("Unhandled SSL error during write [{}]", err));
}

ResultOr<SSL*> AttachSsl(core::FileDescriptor& fd, net::SslContext* sc) {
  TRY_ASSIGN(void* vp_ssl, sc->AllocateSsl());
  auto* const ssl = reinterpret_cast<SSL*>(vp_ssl);

  // Associate the fd & the SSL.
  if (0 == SSL_set_fd(ssl, fd.fd())) {
    SSL_free(ssl);
    return core::StreamFatalError("SSL_set_fd");
  }

  // Need to inform SSL to never close the FD.
  BIO* const b = SSL_get_rbio(ssl);
  if (nullptr == b) {
    // In this nasty condition, fd will be closed inside SSL_free(), so we must
    // prevent fd from double-closign.
    SSL_free(ssl);
    static_cast<void>(fd.Release().ValueOrDie());
    return core::StreamFatalError("SSL_get_rbio");
  }
  BIO_set_shutdown(b, BIO_NOCLOSE);

  return ssl;
}

}  // namespace

SslSocket::SslSocket(PrivateTag, void* ssl, SslHandshaker shs)
    : ssl_(ssl), handshaker_(std::move(shs)) {}

SslSocket::~SslSocket() {
  if (nullptr != ssl_) {
    SSL_free(reinterpret_cast<SSL*>(ssl_));
  }
}

// static
ResultOr<std::shared_ptr<SslSocket>> SslSocket::New(io::Epoller* e,
                                                    net::SslContext* sc,
                                                    core::FileDescriptor fd,
                                                    net::SocketAddress sa,
                                                    SslHandshaker shs) {
  static_cast<void>(sa);  // idk what to do with this yet.
  TRY_ASSIGN(auto* ssl, AttachSsl(fd, sc));
  auto ret = std::make_shared<SslSocket>(PrivateTag{}, ssl, std::move(shs));
  TRY(e->SetNonBlockingAndRegister(ret, std::move(fd)));
  return ret;
}

ResultOr<io::IoHandler::Outcome> SslSocket::AttemptRead() {
  Outcome o = Outcome::kSuspend;
  if (sinks_.empty()) return o;

  auto& s = sinks_.front();
  SSL* const ssl = reinterpret_cast<SSL*>(ssl_);
  TRY_ASSIGN(const auto n, NonBlockingSslRead(&o, h_, ssl, &read_wants_write_,
                                              s.data(), s.size()));
  if (n > 0) {
    static_cast<void>(s.Advance(n));
    sinks_.pop();  // Admit partial fills of the buffer.
  }
  return o;
}

ResultOr<io::IoHandler::Outcome> SslSocket::AttemptWrite() {
  Outcome o = Outcome::kSuspend;
  if (sources_.empty()) return o;

  auto& s = sources_.front();
  SSL* const ssl = reinterpret_cast<SSL*>(ssl_);
  TRY_ASSIGN(const auto n, NonBlockingSslWrite(&o, h_, ssl, &write_wants_read_,
                                               s.data(), s.size()));
  if (s.Consume(n)) {
    sources_.pop();
  }

  if (sources_.empty() && close_requested_) {
    o = Outcome::kClose;
  }
  return o;
}

ResultOr<io::IoHandler::Outcome> SslSocket::HandleReadEstablished() {
  if (write_wants_read_) {
    write_wants_read_ = false;
    io::Context::Current()->RequestWrite(h_);
    // Repeat our last *write* operation. Return a read-yield so reads can
    // proceed. Request a write so that the write state machine in the epoller
    // resynchonizes with the fd.
    TRY_ASSIGN(auto swallowed, AttemptWrite());
    static_cast<void>(swallowed);
    return Outcome::kYield;
  }
  return AttemptRead();
}

ResultOr<io::IoHandler::Outcome> SslSocket::HandleWriteEstablished() {
  if (read_wants_write_) {
    read_wants_write_ = false;
    io::Context::Current()->RequestRead(h_);
    // Repeat our last *read* operation. Return a write-yield so reads can
    // proceed. Request a read so that the read state machine in the epoller
    // resynchronizes with the fd.
    TRY_ASSIGN(auto swallowed, AttemptRead());
    static_cast<void>(swallowed);
    return Outcome::kYield;
  }
  return AttemptWrite();
}

ResultOr<io::IoHandler::Outcome> SslSocket::HandleRead(
    io::FdHandle h, const core::FileDescriptor& fd) {
  h_ = h;
  if (handshaker_.in_progress()) {
    if (!handshaker_.wants_read()) {
      return Outcome::kSuspend;
    }
    TRY_ASSIGN(const auto vibe, handshaker_.RunAccept(ssl_));
    switch (vibe) {
      case SslHandshaker::Vibe::kComplete:
        io::Context::Current()->RequestWrite(h_);  // in case we suspended
        return Outcome::kYield;
      case SslHandshaker::Vibe::kWantedRead:
        return Outcome::kFdEagain;
      case SslHandshaker::Vibe::kWantedWrite:
        io::Context::Current()->RequestWrite(h_);
        return Outcome::kSuspend;
    }
  }
  return HandleReadEstablished();
}

ResultOr<io::IoHandler::Outcome> SslSocket::HandleWrite(
    io::FdHandle h, const core::FileDescriptor& fd) {
  h_ = h;
  if (handshaker_.in_progress()) {
    if (!handshaker_.wants_write()) {
      return Outcome::kSuspend;
    }
    TRY_ASSIGN(const auto vibe, handshaker_.RunAccept(ssl_));
    switch (vibe) {
      case SslHandshaker::Vibe::kComplete:
        io::Context::Current()->RequestRead(h_);  // in case we suspended
        return Outcome::kYield;
      case SslHandshaker::Vibe::kWantedRead:
        io::Context::Current()->RequestRead(h_);
        return Outcome::kSuspend;
      case SslHandshaker::Vibe::kWantedWrite:
        return Outcome::kFdEagain;
    }
  }
  return HandleWriteEstablished();
}

void SslSocket::Post(io::SourceBuffer sb) {
  if (close_requested_) return;
  if (sources_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestWrite(h_);
  }
  sources_.push(std::move(sb));
}

void SslSocket::Post(io::SinkBuffer sb) {
  if (sinks_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestRead(h_);
  }
  sinks_.push(std::move(sb));
}

void SslSocket::PostClose() {
  close_requested_ = true;
  if (sources_.empty() && h_ != io::FdHandle::kInvalid) {
    io::Context::Current()->RequestWrite(h_);
  }
}

}  // namespace net
