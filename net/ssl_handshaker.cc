#include <openssl/err.h>
#include <openssl/ssl.h>

#include "net/ssl_handshaker.h"
#include "net/ssl_helpers.h"

namespace net {

SslHandshaker::SslHandshaker(std::move_only_function<int(void*)> acceptor)
    : acceptor_(std::move(acceptor)) {}

// static
std::move_only_function<int(void*)> SslHandshaker::RealAcceptor() {
  return [](void* vp_ssl) -> int {
    SSL* const ssl = reinterpret_cast<SSL*>(vp_ssl);
    const int ret = SSL_accept(ssl);
    if (1 == ret) return 0;
    return SSL_get_error(ssl, ret);
  };
}

ResultOr<SslHandshaker::Vibe> SslHandshaker::RunAccept(void* vp_ssl) {
  if (!in_progress()) return Vibe::kComplete;
  wants_read_ = false;
  wants_write_ = false;

  // Run acceptor and react.
  const auto err = acceptor_(vp_ssl);
  switch (err) {
    case 0:
      in_progress_ = false;
      return Vibe::kComplete;
    case SSL_ERROR_WANT_READ:
      wants_read_ = true;
      return Vibe::kWantedRead;
    case SSL_ERROR_WANT_WRITE:
      wants_write_ = true;
      return Vibe::kWantedWrite;
    case SSL_ERROR_ZERO_RETURN:
    default:
      in_progress_ = false;
      return GetSslErrorFromThreadLocalQueue();
  }
}

}  // namespace net
