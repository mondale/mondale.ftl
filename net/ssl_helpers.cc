#include <openssl/err.h>

#include "net/ssl_helpers.h"

namespace net {

std::string GetSslErrorStringFromThreadLocalQueue() {
  unsigned long err_code = ERR_get_error();
  if (0 == err_code) return "";

  // Convert error code to a human-readable string
  char err_buf[256];
  ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
  return std::string(err_buf);
}

Result GetSslErrorFromThreadLocalQueue() {
  return core::StreamFatalError(GetSslErrorStringFromThreadLocalQueue());
}

}  // namespace net
