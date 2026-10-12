#include "http/handlers/test_fixture.h"
#include "http/response_builder.h"
#include "testing/testing.h"

namespace {

using http::handlers::testing::ClientType;
using http::handlers::testing::TestFixture;
using testing::HasSubstr;

// A simple mock handler used exclusively to test the TestFixture's behavior.
ResultOr<std::string> MockServeHandler(const http::RequestParser& rp) {
  // Echo back part of the request to verify which client parser is active
  bool is_browser = rp.user_agent().starts_with("Mozilla");

  std::string body = strings::Format(
      "<!DOCTYPE html>\n"
      "<html lang=\"en\">\n"
      "<head>\n"
      "  <meta charset=\"UTF-8\">\n"
      "  <title>mock title</title>\n"
      "</head>\n"
      "<body>\n"
      "  <h1>Client: {}</h1>\n"
      "</body>\n"
      "</html>",
      is_browser ? "Browser" : "NonBrowser");

  http::ResponseBuilder rb;
  rb.SetStatus(200, "OK");
  rb.SetHeader("Content-Type", "text/html; charset=utf-8");
  // Simulating response body construction for the test fixture
  return strings::Format(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n\r\n{}",
      body);
}

// Concrete subclass instantiating TestFixture to test its functionality
class TestFixtureSelfTest : public TestFixture {
 protected:
  TestFixtureSelfTest() : TestFixture("/mock-uri") {}
};

TEST_F(TestFixtureSelfTest, InitializesParsersWithCorrectUserAgents) {
  // Verify that the constructor correctly populated the request parsers
  EXPECT_TRUE(browser_request_parser_.user_agent().starts_with("Mozilla"));
  EXPECT_FALSE(non_browser_request_parser_.user_agent().starts_with("Mozilla"));
  EXPECT_EQ(uri_, "/mock-uri");
}

TEST_F(TestFixtureSelfTest,
       RunOnBothClientsExecutesTwiceWithDifferentContexts) {
  int execution_count = 0;
  bool saw_browser = false;
  bool saw_non_browser = false;

  RunOnBothClients(
      [](const http::RequestParser& rp) { return MockServeHandler(rp); },
      [&](ClientType client_type, const std::string& response) {
        execution_count++;
        if (client_type == ClientType::kBrowser) {
          saw_browser = true;
          EXPECT_THAT(response, HasSubstr("Client: Browser"));
        } else if (client_type == ClientType::kNonBrowser) {
          saw_non_browser = true;
          EXPECT_THAT(response, HasSubstr("Client: NonBrowser"));
        }
      });

  EXPECT_EQ(execution_count, 2);
  EXPECT_TRUE(saw_browser);
  EXPECT_TRUE(saw_non_browser);
}

TEST_F(TestFixtureSelfTest, EnvelopeAndHeaderAssertionsPassOnValidResponse) {
  RunOnBothClients(
      [](const http::RequestParser& rp) { return MockServeHandler(rp); },
      [this](ClientType client_type, const std::string& response) {
        // Exercise the built-in fixture assertion helpers
        AssertStandardHtmlHeaders(response, 200);
        AssertValidHtmlEnvelope(response, "mock title");
        AssertStructuralTagBalance(response);
      });
}

TEST_F(TestFixtureSelfTest, ClientTypeStreamingAndToStringWork) {
  // Verify the custom printing/logging support for ClientType
  EXPECT_EQ(ToString(ClientType::kBrowser), "kBrowser");
  EXPECT_EQ(ToString(ClientType::kNonBrowser), "kNonBrowser");

  std::ostringstream browser_stream;
  browser_stream << ClientType::kBrowser;
  EXPECT_EQ(browser_stream.str(), "kBrowser");

  std::ostringstream non_browser_stream;
  non_browser_stream << ClientType::kNonBrowser;
  EXPECT_EQ(non_browser_stream.str(), "kNonBrowser");
}

}  // namespace
