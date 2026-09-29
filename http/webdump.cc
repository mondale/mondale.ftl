#include "base/flags.h"
#include "base/process.h"
#include "core/vocabulary.h"
#include "http/request_parser.h"
#include "io/io.h"
#include "net/listener.h"
#include "net/ssl_context.h"
#include "net/ssl_socket.h"
#include "net/tls_sniffer.h"

FLAG_COHORT(webdump);
FLAG(uint16_t, port, 8000).Ge(100).Le(65535);

namespace {

Result Dump(const char* buf, size_t n) {
  Log(INFO) << "\n" << strings::Hexdump(buf, n);
  http::RequestParser p;
  TRY(p.Parse({buf, n}));
  Log(INFO) << p.ToString();
  return Result::Ok();
}

class HttpDumper final : public io::IoHandler {
 private:
  struct PrivateTag {};

 public:
  explicit HttpDumper(PrivateTag) {}

  static std::shared_ptr<HttpDumper> Build() {
    return std::make_shared<HttpDumper>(PrivateTag{});
  }

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kSuspend;
    constexpr size_t kSize = 1024;
    char buf[kSize];
    TRY_ASSIGN(auto n, NonBlockingRead(&o, fd, buf, kSize));
    if (n > 0) {
      TRY(Dump(buf, n));
    }
    return o;
  }

  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override {
    return Outcome::kSuspend;
  }
};

Result Webdump() {
  std::unique_ptr<char[]> buf(new char[4096]);
  TRY_ASSIGN(auto sc, net::SslContext::BuildServer());
  TRY_ASSIGN(auto epoller, io::Epoller::Build(1));
  const uint16_t port = FLAG_LOOKUP(port);

  auto classified = [&](bool is_tls, core::FileDescriptor fd,
                        net::SocketAddress sa) {
    if (is_tls) {
      Log(INFO) << "Inbound connection from [" << sa.ToString()
                << "] snoops as TLS.";
      auto s = net::SslSocket::New(epoller.get(), sc.get(), std::move(fd),
                                   std::move(sa), net::SslHandshaker{})
                   .ValueOrDie();
      s->Post(io::SinkBuffer(buf.get(), 4096, [&](size_t n) {
        auto r = Dump(buf.get(), n);
        Log(ERROR, If(!r.IsOk())) << r;
      }));
    } else {
      Log(INFO) << "Inbound connection from [" << sa.ToString()
                << "] snoops as non-TLS.";
      auto h = HttpDumper::Build();
      CHECK_OK(epoller->Register(std::move(h), std::move(fd)));
    }
  };

  TRY_ASSIGN(auto http_listner,
             net::Listener::Build(
                 epoller.get(), port,
                 [&](core::FileDescriptor fd, net::SocketAddress sa) {
                   Log(INFO) << "Inbound connection from: " << sa.ToString();
                   auto snooper =
                       net::TlsSniffer::New(sa, std::move(classified));
                   CHECK_OK(epoller->SetNonBlockingAndRegister(
                       std::move(snooper), std::move(fd)));
                 }));
  Log(INFO) << "HTTP/HTTPS Listener online on port [" << port << "].";
  base::AwaitSigInt();
  Log(INFO) << "CTRL+C'd, exiting.";

  return Result::Ok();
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  core::util::DieElegantlyIfNotOk(Webdump());
  return EXIT_SUCCESS;
}
