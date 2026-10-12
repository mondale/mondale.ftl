#include "http/handlers/procz.h"
#include "http/handlers/test_fixture.h"
#include "http/request_parser.h"
#include "testing/testing.h"

namespace {

using http::RequestParser;
using http::handlers::testing::ClientType;
using http::handlers::testing::TestFixture;
using testing::HasSubstr;

class ProczHandlerTest : public TestFixture {
 protected:
  ProczHandlerTest() : TestFixture("/procz") {}
};

TEST_F(ProczHandlerTest, ServesIndexWhenNoFileParamProvided) {
  RunOnBothClients(
      [](const RequestParser& rp) { return http::handlers::ServeProcz(rp); },
      [this](ClientType client_type, const std::string& response) {
        AssertStandardHtmlHeaders(response, 200);
        AssertValidHtmlEnvelope(response, "procz");
        AssertStructuralTagBalance(response);

        // Verify index links are present
        EXPECT_THAT(response, HasSubstr("<h1>procz</h1>"));
        EXPECT_THAT(response, HasSubstr("href=\"/procz?file=cpuinfo\""));
        EXPECT_THAT(response, HasSubstr("href=\"/procz?file=loadavg\""));
      });
}

TEST_F(ProczHandlerTest, ServesValidFileContentWhenRequested) {
  // Construct a request parser pointing to /procz?file=cpuinfo
  RequestParser file_request;
  std::string raw_request =
      "GET /procz?file=cpuinfo HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "User-Agent: Mozilla/5.0 TestBrowser/1.0\r\n\r\n";

  ASSERT_TRUE(file_request.Parse(raw_request).IsOk());

  ResultOr<std::string> result = http::handlers::ServeProcz(file_request);
  ASSERT_TRUE(result.IsOk());
  std::string response = result.ValueOrDie();

  // Verify headers and envelope for the file response
  AssertStandardHtmlHeaders(response, 200);
  EXPECT_THAT(response, HasSubstr("<title>cpuinfo -> /proc/cpuinfo</title>"));
  EXPECT_THAT(response, HasSubstr("<pre>"));
}

TEST_F(ProczHandlerTest, ReturnsNotFoundForInvalidFileParam) {
  RequestParser invalid_request;
  std::string raw_request =
      "GET /procz?file=nonexistent_metric HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "User-Agent: Mozilla/5.0 TestBrowser/1.0\r\n\r\n";

  ASSERT_TRUE(invalid_request.Parse(raw_request).IsOk());

  ResultOr<std::string> result = http::handlers::ServeProcz(invalid_request);
  ASSERT_TRUE(result.IsOk());
  std::string response = result.ValueOrDie();

  // Should trigger the 404 CannedResponse path
  EXPECT_THAT(response, HasSubstr("404"));
}

}  // namespace
