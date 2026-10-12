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

io::SourceBuffer Browser404(const RequestParser& rp) {
  ResponseBuilder hrb;
  hrb.SetStatus(404, "Not Found");
  hrb.RespondingTo(rp);
  hrb.SetHeader("Content-Type", "text/html; charset=UTF-8");
  hrb.SetHeader("Connection", "close");

  hrb.Doctype();
  hrb.Html({{"lang", "en"}}, [&] {
    hrb.Head([&] {
      hrb.Meta({{"charset", "UTF-8"}});
      hrb.Title("404 - Not Found");
    });
    hrb.Body([&] {
      hrb.Div({{"class", "container"}}, [&] {
        hrb.H1("404");
        hrb.P("No such page.");
        hrb.A({{"href", "/"}}, "Back");
      });
    });
  });

  return FromString(hrb.ToString());
}

}  // namespace

CannedResponses::CannedResponses(PrivateTag) {}

// static
CannedResponses* CannedResponses::Instance() {
  static CannedResponses* cr = new CannedResponses(PrivateTag{});
  return cr;
}

io::SourceBuffer CannedResponses::NotFound404(const RequestParser& rp) const {
  if (IsBrowser(rp)) return Browser404(rp);
  return io::SourceBuffer(k404TextResponse, sizeof(k404TextResponse), []() {});
}

}  // namespace http
