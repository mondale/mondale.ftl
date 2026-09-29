#include "net/ssl_context.h"
#include "testing/testing.h"

namespace net {

TEST(EmptyServerTest) { auto c = SslContext::BuildServer().ValueOrDie(); }

TEST(EmptyClientTest) { auto c = SslContext::BuildClient().ValueOrDie(); }

}  // namespace net
