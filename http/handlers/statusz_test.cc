#include <functional>

#include "http/handlers/statusz.h"
#include "http/handlers/test_fixture.h"
#include "http/request_parser.h"
#include "testing/testing.h"

using http::handlers::testing::ClientType;
using http::handlers::testing::TestFixture;
using testing::HasSubstr;

namespace {

// Specialized test fixture for the statusz handler passing its URI upward
class StatuszHandlerTest : public TestFixture {
 protected:
  StatuszHandlerTest() : TestFixture("/statusz") {}
};

TEST_F(StatuszHandlerTest, GeneratesValidHtmlStructureAcrossClients) {
  RunOnBothClients(
      [](const http::RequestParser& rp) {
        return http::handlers::ServeStatusz(rp);
      },
      [this](ClientType client_type, const std::string& response) {
        AssertStandardHtmlHeaders(response, 200);
        AssertValidHtmlEnvelope(response, "statusz");
        AssertStructuralTagBalance(response);

        EXPECT_THAT(response, HasSubstr("<h1>statusz</h1>"));
      });
}

TEST_F(StatuszHandlerTest, IncludesDynamicProcessMetricsAcrossClients) {
  RunOnBothClients(
      [](const http::RequestParser& rp) {
        return http::handlers::ServeStatusz(rp);
      },
      [](ClientType client_type, const std::string& response) {
        EXPECT_THAT(response, HasSubstr("Started at"));
        EXPECT_THAT(response, HasSubstr("Up for"));
      });
}

}  // namespace
