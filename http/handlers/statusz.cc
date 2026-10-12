#include "base/process.h"
#include "http/handlers/statusz.h"
#include "http/http_server.h"
#include "http/response_builder.h"

namespace http::handlers {

ResultOr<std::string> ServeStatusz(const RequestParser& rp) {
  ResponseBuilder rb;
  rb.SetStatus(200, "OK");
  rb.SetHeader("Content-Type", "text/html; charset=utf-8");
  rb.RespondingTo(rp);
  rb.Doctype();
  rb.Html({{"lang", "en"}}, [&] {
    rb.Head([&] {
      rb.Meta({{"charset", "UTF-8"}});
      rb.Title("statusz");
    });
    rb.Body([&] {
      rb.H1("statusz");
      rb.P(strings::Format("Started at {}",
                           base::ProcessStartTime().ToString()));
      rb.P(strings::Format(
          "Up for {}", core::strings::HumanReadable(base::ProcessUptime())));
    });
  });
  return rb.ToString();
}

static bool reg = RegisterGlobalUriHandler(
    "/statusz", [](const RequestParser& rp) { return ServeStatusz(rp); });

}  // namespace http::handlers
