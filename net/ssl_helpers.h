#ifndef NET_SSL_HELPERS_H_
#define NET_SSL_HELPERS_H_

#include "core/vocabulary.h"

namespace net {

std::string GetSslErrorStringFromThreadLocalQueue();
Result GetSslErrorFromThreadLocalQueue();

}  // namespace net

#endif  // #ifndef NET_SSL_HELPERS_H_
