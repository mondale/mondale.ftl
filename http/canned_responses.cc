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
constexpr char kBrowserStyle[] =
    "\n        body {\n"
    "            background-color: #1e1e1e;\n"
    "            color: #ffffff;\n"
    "            font-family: monospace;\n"
    "            display: flex;\n"
    "            flex-direction: column;\n"
    "            align-items: center;\n"
    "            justify-content: center;\n"
    "            height: 100vh;\n"
    "            margin: 0;\n"
    "        }\n"
    "        .container {\n"
    "            text-align: center;\n"
    "            padding: 2.5rem;\n"
    "            border: 1px solid #333333;\n"
    "            border-radius: 4px;\n"
    "            background-color: #252526;\n"
    "            max-width: 400px;\n"
    "        }\n"
    "        h1 {\n"
    "            font-size: 1.8rem;\n"
    "            margin-top: 0;\n"
    "            margin-bottom: 0.5rem;\n"
    "            color: #569cd6;\n"
    "        }\n"
    "        p {\n"
    "            color: #cccccc;\n"
    "            margin-bottom: 1.5rem;\n"
    "            line-height: 1.4;\n"
    "        }\n"
    "        a {\n"
    "            color: #569cd6;\n"
    "            text-decoration: none;\n"
    "            border-bottom: 1px dotted #569cd6;\n"
    "        }\n"
    "        a:hover {\n"
    "            color: #ffffff;\n"
    "            border-bottom: 1px solid #ffffff;\n"
    "        }\n";

bool IsBrowser(const RequestParser& rp) {
  return strings::StartsWith(rp.user_agent(), "Mozilla");
}

io::SourceBuffer FromString(std::string s) {
  auto* const d = s.data();
  const size_t n = s.length();
  return io::SourceBuffer(d, n, [x = std::move(s)]() {});
}

io::SourceBuffer Browser404() {
  ResponseBuilder hrb;
  hrb.SetStatus(404, "Not Found");
  hrb.SetHeader("Content-Type", "text/html; charset=UTF-8");
  hrb.SetHeader("Connection", "close");

  hrb.Doctype();
  hrb.Html({{"lang", "en"}}, [&] {
    hrb.Head([&] {
      hrb.Meta({{"charset", "UTF-8"}});
      hrb.Title("404 - Not Found");
      hrb.Style(kBrowserStyle);
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
  if (IsBrowser(rp)) return Browser404();
  return io::SourceBuffer(k404TextResponse, sizeof(k404TextResponse), []() {});
}

}  // namespace http
