#ifndef NET_SSL_HANDSHAKER_H_
#define NET_SSL_HANDSHAKER_H_

#include <functional>

#include "core/vocabulary.h"

namespace net {

// Encapsulates the logic associated with an SSL handshake.
class SslHandshaker final {
 public:
  explicit SslHandshaker(
      std::move_only_function<int(void*)> acceptor = RealAcceptor());

  static std::move_only_function<int(void*)> RealAcceptor();

  // TODO need a macro for this
  SslHandshaker(SslHandshaker&&) noexcept = default;
  SslHandshaker& operator=(SslHandshaker&&) noexcept = default;

  enum class Vibe { kComplete, kWantedRead, kWantedWrite };

  ResultOr<Vibe> RunAccept(void* vp_ssl);

  bool in_progress() const { return in_progress_; }
  bool wants_read() const { return wants_read_; }
  bool wants_write() const { return wants_write_; }

 private:
  std::move_only_function<int(void*)> acceptor_;
  bool in_progress_ = true;
  bool wants_read_ = true;
  bool wants_write_ = false;
};

}  // namespace net

#endif  // #ifndef NET_SSL_HANDSHAKER_H_
