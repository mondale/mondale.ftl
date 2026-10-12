#include "http/canned_responses.h"
#include "http/response_builder.h"

namespace http {
namespace {

constexpr char k404TextResponse[] =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Content-Length: 9\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Not Found";

bool IsBrowser(const RequestParser& rp) {
  return strings::StartsWith(rp.user_agent(), "Mozilla");
}

io::SourceBuffer FromString(std::string s) {
  auto* const d = s.data();
  const size_t n = s.length();
  return io::SourceBuffer(d, n, [x = std::move(s)]() {});
}

void Common404(ResponseBuilder& rb, std::string_view hint) {
  rb.SetStatus(404, "Not Found");
  rb.SetHeader("Content-Type", "text/html; charset=UTF-8");
  rb.SetHeader("Connection", "close");
  rb.Doctype();
  rb.Html({{"lang", "en"}}, [&] {
    rb.Head([&] {
      rb.Meta({{"charset", "UTF-8"}});
      rb.Title("404 - Not Found");
    });
    rb.Body([&] {
      rb.Div({{"class", "container"}}, [&] {
        rb.H1("404");
        if (hint.empty()) {
          rb.P("No such page.");
        } else {
          rb.P(strings::Format("The requested page [{}] could not be found.",
                               hint));
        }
        rb.A({{"href", "/"}}, "Back");
      });
    });
  });
}

io::SourceBuffer Browser404(const RequestParser& rp) {
  ResponseBuilder hrb;
  hrb.RespondingTo(rp);
  Common404(hrb, rp.path());
  return FromString(hrb.ToString());
}

}  // namespace

CannedResponses::CannedResponses(PrivateTag) {}

// static
CannedResponses* CannedResponses::Instance() {
  static CannedResponses* cr = new CannedResponses(PrivateTag{});
  return cr;
}

void CannedResponses::NotFound404(ResponseBuilder& rb,
                                  std::string_view hint) const {
  Common404(rb, hint);
}

io::SourceBuffer CannedResponses::NotFound404(const RequestParser& rp) const {
  if (IsBrowser(rp)) return Browser404(rp);
  return io::SourceBuffer(k404TextResponse, sizeof(k404TextResponse), []() {});
}

}  // namespace http
