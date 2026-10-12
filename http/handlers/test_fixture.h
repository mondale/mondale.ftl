#ifndef HTTP_HANDLERS_TEST_FIXTURE_H_
#define HTTP_HANDLERS_TEST_FIXTURE_H_

#include <functional>

#include "core/vocabulary.h"
#include "http/request_parser.h"
#include "testing/testing.h"

namespace http::handlers::testing {

// Client context type passed to test execution helpers
enum class ClientType {
  kBrowser,
  kNonBrowser,
};

std::string ToString(ClientType client_type);
std::ostream& operator<<(std::ostream& os, ClientType client_type);

// Reusable base fixture for HTML-generating handlers that tests both
// browser and non-browser client requests, initialized via the constructor.
class TestFixture : public ::testing::Test {
 protected:
  explicit TestFixture(std::string_view uri);

  // Invokes the provided handler callback once for the pre-configured browser
  // client and once for the non-browser client
  void RunOnBothClients(
      const std::function<ResultOr<std::string>(const http::RequestParser&)>&
          handler,
      const std::function<void(ClientType client_type,
                               const std::string& response)>& test_body);

  // Verifies standard HTTP protocol headers and response status
  void AssertStandardHtmlHeaders(const std::string& response,
                                 int expected_status = 200);

  // Generic helper for standard HTML boilerplate and document envelope
  void AssertValidHtmlEnvelope(const std::string& response,
                               std::string_view expected_title);

  // Generic HTML correctness test: ensures core structural tags open and close
  // correctly
  void AssertStructuralTagBalance(const std::string& response);

 private:
  void VerifyTagPresentAndClosed(const std::string& text, std::string_view tag);

 protected:
  std::string uri_;
  http::RequestParser browser_request_parser_;
  http::RequestParser non_browser_request_parser_;
};

}  // namespace http::handlers::testing

#endif  // #ifndef HTTP_HANDLERS_TEST_FIXTURE_H_
