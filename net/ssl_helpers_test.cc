#include <openssl/err.h>

#include "net/ssl_helpers.h"
#include "testing/testing.h"

namespace {

class SslHelpersTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure the thread-local error queue is clean before each test
    ERR_clear_error();
  }
};

TEST_F(SslHelpersTest, EmptyErrorQueueReturnsEmptyStringAndErrorResult) {
  // When the queue is empty, string extraction should return empty
  std::string err_str = net::GetSslErrorStringFromThreadLocalQueue();
  EXPECT_EQ(err_str, "");

  // The Result wrapper should return a non-OK stream fatal error
  Result res = net::GetSslErrorFromThreadLocalQueue();
  EXPECT_FALSE(res.IsOk());
}

TEST_F(SslHelpersTest, PopulatedErrorQueueFormatsCorrectly) {
  // Manually push a mock error into OpenSSL's thread-local error queue
  // Signature: ERR_put_error(lib, func, reason, file, line)
  ERR_put_error(ERR_LIB_USER, 0, 42, "ssl_helpers_test.cc", __LINE__);

  std::string err_str = net::GetSslErrorStringFromThreadLocalQueue();
  EXPECT_FALSE(err_str.empty());
  EXPECT_THAT(err_str, testing::HasSubstr("lib"));
  EXPECT_THAT(err_str, testing::HasSubstr("42"));

  // Push another to test the Result factory helper
  ERR_put_error(ERR_LIB_USER, 0, 43, "ssl_helpers_test.cc", __LINE__);
  Result res = net::GetSslErrorFromThreadLocalQueue();
  EXPECT_FALSE(res.IsOk());
}

}  // namespace
