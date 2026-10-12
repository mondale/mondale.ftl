#include "http/canned_responses.h"
#include "http/request_parser.h"
#include "io/io.h"
#include "testing/testing.h"

namespace {

using http::CannedResponses;
using http::RequestParser;
using testing::HasSubstr;

// Helper to convert an io::SourceBuffer into a std::string_view for inspection.
std::string_view AsStringView(const io::SourceBuffer& buffer) {
  return std::string_view(buffer.data(), buffer.size());
}

TEST(CannedResponsesTest_InstanceReturnsSingleton) {
  CannedResponses* instance1 = CannedResponses::Instance();
  CannedResponses* instance2 = CannedResponses::Instance();

  ASSERT_TRUE(instance1 != nullptr);
  EXPECT_EQ(instance1, instance2);
}

TEST(CannedResponsesTest_NotFound404ReturnsPlaintextForNonBrowser) {
  RequestParser parser;
  // A default or non-Mozilla user agent should result in the plain text 404
  // response.
  io::SourceBuffer buffer = CannedResponses::Instance()->NotFound404(parser);
  std::string_view content = AsStringView(buffer);

  EXPECT_THAT(content, HasSubstr("HTTP/1.1 404 Not Found"));
  EXPECT_THAT(content, HasSubstr("Content-Type: text/plain"));
  EXPECT_THAT(content, HasSubstr("Not Found"));
  EXPECT_THAT(content, Not(HasSubstr("text/html")));
}

TEST(CannedResponsesTest_NotFound404ReturnsHtmlForBrowser) {
  RequestParser parser;
  static_cast<void>(parser
                        .Parse("GET /nonexistent HTTP/1.1\r\n"
                               "Host: localhost\r\n"
                               "User-Agent: Mozilla/5.0 (X11; Linux x86_64; "
                               "rv:109.0) Gecko/20100101 "
                               "Firefox/115.0\r\n\r\n")
                        .ValueOrDie());

  io::SourceBuffer buffer = CannedResponses::Instance()->NotFound404(parser);
  std::string_view content = AsStringView(buffer);

  EXPECT_THAT(content, HasSubstr("404 - Not Found"));
  EXPECT_THAT(content, HasSubstr("Content-Type: text/html; charset=UTF-8"));
  EXPECT_THAT(content, HasSubstr("nonexistent"));
  EXPECT_THAT(content, HasSubstr("<div class=\"container\">"));
}

}  // namespace
