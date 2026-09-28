#include <string>

#include "http/request_parser.h"
#include "testing/testing.h"

namespace {

using http::RequestParser;

class RequestParserTest : public ::testing::Test {
 protected:
  RequestParser parser_;
};

TEST_F(RequestParserTest, ParsesSimpleGetRequest) {
  std::string_view raw_request =
      "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";

  ResultOr<size_t> result = parser_.Parse(raw_request);
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.ValueOrDie(), raw_request.size());

  EXPECT_EQ(parser_.method(), RequestParser::Method::kGet);
  EXPECT_EQ(parser_.version(), RequestParser::Version::kHttp1point1);
  EXPECT_EQ(parser_.uri(), "/index.html");
  EXPECT_TRUE(parser_.body().empty());

  auto it = parser_.keyvals().find("Host");
  ASSERT_TRUE(it != parser_.keyvals().end());
  EXPECT_EQ(it->second, "localhost");
}

TEST_F(RequestParserTest, ParsesPostRequestWithBody) {
  std::string_view raw_request =
      "POST /submit HTTP/1.1\r\n"
      "Content-Length: 13\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Hello, World!";

  ResultOr<size_t> result = parser_.Parse(raw_request);
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.ValueOrDie(), raw_request.size());

  EXPECT_EQ(parser_.method(), RequestParser::Method::kPost);
  EXPECT_EQ(parser_.version(), RequestParser::Version::kHttp1point1);
  EXPECT_EQ(parser_.uri(), "/submit");
  EXPECT_EQ(parser_.body(), "Hello, World!");
}

TEST_F(RequestParserTest, RequestsMoreDataWhenIncomplete) {
  std::string_view partial_request =
      "GET /index.html HTTP/1.1\r\nHost: localhost\r\n";

  ResultOr<size_t> result = parser_.Parse(partial_request);
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.ValueOrDie(), 0);
}

TEST_F(RequestParserTest, FailsOnInvalidMethod) {
  std::string_view raw_request = "INVALID /index.html HTTP/1.1\r\n\r\n";

  ResultOr<size_t> result = parser_.Parse(raw_request);
  EXPECT_FALSE(result.IsOk());
}

TEST_F(RequestParserTest, FailsOnInvalidVersion) {
  std::string_view raw_request = "GET /index.html HTTP/0.9\r\n\r\n";

  ResultOr<size_t> result = parser_.Parse(raw_request);
  EXPECT_FALSE(result.IsOk());
}

TEST_F(RequestParserTest, FailsOnContentLengthForGetRequest) {
  std::string_view raw_request =
      "GET /index.html HTTP/1.1\r\n"
      "Content-Length: 5\r\n"
      "\r\n"
      "hello";

  ResultOr<size_t> result = parser_.Parse(raw_request);
  EXPECT_FALSE(result.IsOk());
}

TEST(RequestParserFormattingTest_MethodToString) {
  EXPECT_EQ(ToString(RequestParser::Method::kGet), "GET");
  EXPECT_EQ(ToString(RequestParser::Method::kPost), "POST");
  EXPECT_EQ(ToString(RequestParser::Method::kHead), "HEAD");
  EXPECT_EQ(ToString(RequestParser::Method::kDelete), "DELETE");
  EXPECT_EQ(ToString(RequestParser::Method::kOptions), "OPTIONS");
  EXPECT_EQ(ToString(RequestParser::Method::kTrace), "TRACE");
  EXPECT_EQ(ToString(RequestParser::Method::kConnect), "CONNECT");
  EXPECT_EQ(ToString(RequestParser::Method::kPut), "PUT");
  EXPECT_EQ(ToString(RequestParser::Method::kPatch), "PATCH");
}

TEST(RequestParserFormattingTest_VersionToString) {
  EXPECT_EQ(ToString(RequestParser::Version::kHttp1point1), "HTTP/1.1");
  EXPECT_EQ(ToString(RequestParser::Version::kHttp2point0), "HTTP/2.0");
}

TEST(RequestParserFormattingTest_MethodStreamOperator) {
  std::ostringstream oss;
  oss << RequestParser::Method::kPost;
  EXPECT_EQ(oss.str(), "POST");
}

TEST(RequestParserFormattingTest_VersionStreamOperator) {
  std::ostringstream oss;
  oss << RequestParser::Version::kHttp1point1;
  EXPECT_EQ(oss.str(), "HTTP/1.1");
}

}  // namespace
