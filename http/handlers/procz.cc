#include "core/file.h"
#include "http/canned_responses.h"
#include "http/handlers/procz.h"
#include "http/http_server.h"
#include "http/response_builder.h"

namespace http::handlers {
namespace {

struct ProcMap final {
  std::string_view prefix;
  std::string_view path;
};

constexpr std::array<ProcMap, 4> kProcMap = {{
    {"cmdline", "/proc/self/cmdline"},
    {"cpuinfo", "/proc/cpuinfo"},
    {"limits", "/proc/self/limits"},
    {"loadavg", "/proc/loadavg"},
}};

std::string_view PrefixToFile(std::string_view p) {
  for (int i = 0; i < kProcMap.size(); ++i) {
    if (kProcMap[i].prefix == p) return kProcMap[i].path;
  }
  return "";
}

ResultOr<std::string> ServeProczIndex(const RequestParser& rp) {
  ResponseBuilder rb;
  rb.SetStatus(200, "OK");
  rb.SetHeader("Content-Type", "text/html; charset=utf-8");
  rb.RespondingTo(rp);
  rb.Doctype();
  rb.Html({{"lang", "en"}}, [&] {
    rb.Head([&] {
      rb.Meta({{"charset", "UTF-8"}});
      rb.Title("procz");
    });
    rb.Body([&] {
      rb.H1("procz");
      for (int i = 0; i < kProcMap.size(); ++i) {
        rb.A({{"href", strings::Format("/procz?file={}", kProcMap[i].prefix)}},
             kProcMap[i].prefix);
      }
    });
  });
  return rb.ToString();
}

ResultOr<std::string> ServeProczFile(std::string_view p,
                                     const RequestParser& rp) {
  ResponseBuilder rb;
  rb.SetHeader("Content-Type", "text/html; charset=utf-8");
  rb.RespondingTo(rp);
  auto f = PrefixToFile(p);
  if (f.empty()) {
    CannedResponses::Instance()->NotFound404(rb, p);
    return rb.ToString();
  }

  Log(INFO) << "Serving [" << f << "].";

  ResultOr<std::string> contents = core::ReadContentsFromFile(f);
  if (!contents.IsOk()) {
    CannedResponses::Instance()->NotFound404(rb, contents.result().ToString());
    return rb.ToString();
  }

  rb.SetStatus(200, "OK");
  rb.Html({{"lang", "en"}}, [&] {
    rb.Head([&] {
      rb.Meta({{"charset", "UTF-8"}});
      rb.Title(strings::Format("{} -> {}", p, f));
    });
    rb.Body([&] { rb.Pre(contents.ValueOrDie()); });
  });

  return rb.ToString();
}

}  // namespace

ResultOr<std::string> ServeProcz(const RequestParser& rp) {
  // Check to see if this is requesting the procz index, or a specific file.
  auto f = rp.query_params().find("file");
  if (f == rp.query_params().end()) return ServeProczIndex(rp);
  return ServeProczFile(f->second, rp);
}

static bool reg = RegisterGlobalUriHandler(
    "/procz", [](const RequestParser& rp) { return ServeProcz(rp); });

}  // namespace http::handlers
