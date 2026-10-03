#include "base/flags.h"
#include "base/process.h"
#include "core/vocabulary.h"
#include "http/http_server.h"

FLAG_COHORT(webserver);
FLAG(uint16_t, port, 8000).Ge(100).Le(65535);

namespace {

Result Run() {
  const uint16_t port = FLAG_LOOKUP(port);
  TRY_ASSIGN(auto hs, http::HttpServer::Create(port));
  Log(INFO) << "HTTP/HTTPS server online on port [" << port << "].";
  base::AwaitSigInt();
  Log(INFO) << "CTRL+C'd, exiting.";

  return Result::Ok();
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  core::util::DieElegantlyIfNotOk(Run());
  return EXIT_SUCCESS;
}
