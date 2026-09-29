#ifndef NET_SSL_CONTEXT_H_
#define NET_SSL_CONTEXT_H_

#include "core/vocabulary.h"

namespace net {

// Generate a private key:
// openssl genpkey -algorithm RSA -out server.key -pkeyopt rsa_keygen_bits:2048
//
// Generate a self-signed key with a 365d expiration:
// openssl req -new -x509 -days 365 -key server.key -out server.crt
class SslContext final {
 public:
  explicit SslContext(void* c) : context_(c) {}
  ~SslContext();

  static ResultOr<std::unique_ptr<SslContext>> BuildServer();
  static ResultOr<std::unique_ptr<SslContext>> BuildClient();

  ResultOr<void*> AllocateSsl();

 private:
  void* context_ = nullptr;
};

}  // namespace net

#endif  // #ifndef NET_SSL_CONTEXT_H_
