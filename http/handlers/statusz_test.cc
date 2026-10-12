#include <functional>

#include "http/handlers/statusz.h"
#include "http/request_parser.h"
#include "testing/testing.h"

using testing::HasSubstr;

namespace {

// Reusable base fixture for HTML-generating handlers that tests both
// browser and non-browser client requests, initialized via the constructor.
class HtmlHandlerTestFixture : public ::testing::Test {
 protected:
  explicit HtmlHandlerTestFixture(std::string_view uri) : uri_(uri) {
    // 1. Initialize browser request parser (User-Agent starting with Mozilla)
    std::string browser_raw_request = strings::Format(
        "GET {} HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "TestBrowser/1.0\r\n"
        "Accept: text/html\r\n\r\n",
        uri_);

    ResultOr<size_t> browser_parse_result =
        browser_request_parser_.Parse(browser_raw_request);
    CHECK(browser_parse_result.IsOk())
        << "Failed to parse browser request for URI: " << uri_;

    // 2. Initialize non-browser request parser (e.g., automated client/script)
    std::string non_browser_raw_request = strings::Format(
        "GET {} HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: ScriptClient/1.0\r\n"
        "Accept: text/html\r\n\r\n",
        uri_);

    ResultOr<size_t> non_browser_parse_result =
        non_browser_request_parser_.Parse(non_browser_raw_request);
    CHECK(non_browser_parse_result.IsOk())
        << "Failed to parse non-browser request for URI: " << uri_;
  }

  // Client context type passed to test execution helpers
  enum class ClientType {
    kBrowser,
    kNonBrowser,
  };

  // Invokes the provided handler callback once for the pre-configured browser
  // client and once for the non-browser client
  void RunOnBothClients(
      const std::function<void(ClientType client_type,
                               const std::string& response)>& test_body) {
    // Test browser case
    {
      ResultOr<std::string> result =
          http::handlers::ServeStatusz(browser_request_parser_);
      ASSERT_TRUE(result.IsOk());
      test_body(ClientType::kBrowser, result.ValueOrDie());
    }

    // Test non-browser case
    {
      ResultOr<std::string> result =
          http::handlers::ServeStatusz(non_browser_request_parser_);
      ASSERT_TRUE(result.IsOk());
      test_body(ClientType::kNonBrowser, result.ValueOrDie());
    }
  }

  // Verifies standard HTTP protocol headers and response status
  void AssertStandardHtmlHeaders(const std::string& response,
                                 int expected_status = 200) {
    std::string status_line = strings::Format("HTTP/1.1 {} ", expected_status);
    EXPECT_THAT(response, HasSubstr(status_line));
    EXPECT_THAT(response, HasSubstr("Content-Type: text/html; charset=utf-8"));
  }

  // Generic helper for standard HTML boilerplate and document envelope
  void AssertValidHtmlEnvelope(const std::string& response,
                               std::string_view expected_title) {
    EXPECT_THAT(response, HasSubstr("<!DOCTYPE html>"));
    EXPECT_THAT(response, HasSubstr("<html lang=\"en\">"));
    EXPECT_THAT(response, HasSubstr("<head>"));

    if (!expected_title.empty()) {
      std::string title_tag =
          strings::Format("<title>{}</title>", expected_title);
      EXPECT_THAT(response, HasSubstr(title_tag));
    }

    EXPECT_THAT(response, HasSubstr("</head>"));
    EXPECT_THAT(response, HasSubstr("<body>"));
    EXPECT_THAT(response, HasSubstr("</body>"));
    EXPECT_THAT(response, HasSubstr("</html>"));
  }

  // Generic HTML correctness test: ensures core structural tags open and close
  // correctly
  void AssertStructuralTagBalance(const std::string& response) {
    VerifyTagPresentAndClosed(response, "html");
    VerifyTagPresentAndClosed(response, "head");
    VerifyTagPresentAndClosed(response, "body");
  }

 private:
  void VerifyTagPresentAndClosed(const std::string& text,
                                 std::string_view tag) {
    std::string open_tag = strings::Format("<{}", tag);
    std::string close_tag = strings::Format("</{}>", tag);

    size_t open_pos = text.find(open_tag);
    size_t close_pos = text.find(close_tag);

    ASSERT_NE(open_pos, std::string::npos)
        << "Missing opening tag: <" << tag << ">";
    ASSERT_NE(close_pos, std::string::npos)
        << "Missing closing tag: </" << tag << ">";
    EXPECT_LT(open_pos, close_pos)
        << "Tag <" << tag << "> was closed before it was opened.";
  }

 protected:
  std::string uri_;
  http::RequestParser browser_request_parser_;
  http::RequestParser non_browser_request_parser_;
};

// Specialized test fixture for the statusz handler passing its URI upward
class StatuszHandlerTest : public HtmlHandlerTestFixture {
 protected:
  StatuszHandlerTest() : HtmlHandlerTestFixture("/statusz") {}
};

TEST_F(StatuszHandlerTest, GeneratesValidHtmlStructureAcrossClients) {
  RunOnBothClients([this](ClientType client_type, const std::string& response) {
    AssertStandardHtmlHeaders(response, 200);
    AssertValidHtmlEnvelope(response, "statusz");
    AssertStructuralTagBalance(response);

    EXPECT_THAT(response, HasSubstr("<h1>statusz</h1>"));
  });
}

TEST_F(StatuszHandlerTest, IncludesDynamicProcessMetricsAcrossClients) {
  RunOnBothClients([](ClientType client_type, const std::string& response) {
    EXPECT_THAT(response, HasSubstr("Started at"));
    EXPECT_THAT(response, HasSubstr("Up for"));
  });
}

}  // namespace
