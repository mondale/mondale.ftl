#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "net/ssl_context.h"
#include "net/ssl_helpers.h"

namespace net {
namespace {

static constexpr uint8_t kCertificate[] = {
#embed "net/keys/server.crt"
};

static constexpr uint8_t kKey[] = {
#embed "net/keys/server.key"
};

Result LoadEmbeddedCredentials(SSL_CTX* ctx) {
  // --- Load Certificate ---
  // Create a read-only memory BIO pointing to the embedded buffer
  BIO* cert_bio = BIO_new_mem_buf(kCertificate, sizeof(kCertificate));
  if (!cert_bio) return core::StreamFatalError("BIO_new_mem_buf");

  // Parse PEM-formatted certificate from the memory BIO
  X509* cert = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
  BIO_free(cert_bio);  // Safe to free BIO immediately after parsing

  if (!cert) return core::StreamFatalError("PEM_read_bio_X509");

  // Attach the parsed X509 object to the SSL context
  if (SSL_CTX_use_certificate(ctx, cert) != 1) {
    X509_free(cert);
    return core::StreamFatalError("SSL_CTX_use_certificate");
  }

  // IMPORTANT: SSL_CTX_use_certificate increments the X509 object's internal
  // reference count, so you must free your local reference to avoid a leak.
  X509_free(cert);

  // --- Load Private Key ---
  BIO* key_bio = BIO_new_mem_buf(kKey, sizeof(kKey));
  if (!key_bio) return core::StreamFatalError("BIO_new_mem_buf");

  // Parse PEM-formatted private key (pass a password string if the key is
  // encrypted)
  EVP_PKEY* pkey = PEM_read_bio_PrivateKey(key_bio, nullptr, nullptr, nullptr);
  BIO_free(key_bio);

  if (!pkey) return core::StreamFatalError("BIO_read_bio_PrivateKey");

  // Attach the private key to the SSL context
  if (SSL_CTX_use_PrivateKey(ctx, pkey) != 1) {
    EVP_PKEY_free(pkey);
    return core::StreamFatalError("SSL_CTX_use_PrivateKey");
  }
  EVP_PKEY_free(pkey);  // Free local reference (context holds its own)

  // --- Sanity Check ---
  if (!SSL_CTX_check_private_key(ctx)) {
    return core::StreamFatalError("Key doesn't match certificate.");
  }

  return Result::Ok();
}

}  // namespace

SslContext::~SslContext() {
  if (nullptr != context_) {
    SSL_CTX_free(reinterpret_cast<SSL_CTX*>(context_));
  }
}

ResultOr<void*> SslContext::AllocateSsl() {
  auto* ssl = SSL_new(reinterpret_cast<SSL_CTX*>(context_));
  if (nullptr == ssl) {
    return GetSslErrorFromThreadLocalQueue();
  }
  return ssl;
}

// static
ResultOr<std::unique_ptr<SslContext>> SslContext::BuildServer() {
  auto* const ctx = SSL_CTX_new(TLS_server_method());
  if (nullptr == ctx) return core::StreamFatalError("SSL_CTX_new");

  // Hardening and configuration.
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 |
                               SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION |
                               SSL_OP_CIPHER_SERVER_PREFERENCE);

  TRY(LoadEmbeddedCredentials(ctx));
  return std::make_unique<SslContext>(ctx);
}

// static
ResultOr<std::unique_ptr<SslContext>> SslContext::BuildClient() {
  auto* const ctx = SSL_CTX_new(TLS_client_method());
  if (nullptr == ctx) return core::StreamFatalError("SSL_CTX_new");
  return std::make_unique<SslContext>(ctx);
}

}  // namespace net
