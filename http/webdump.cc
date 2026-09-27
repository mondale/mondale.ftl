#include "base/flags.h"
#include "base/process.h"
#include "core/vocabulary.h"
#include "io/io.h"
#include "net/listener.h"

FLAG_COHORT(webdump);
FLAG(uint16_t, port, -1).Ge(100).Le(65535);

namespace {

class Dumper final : public io::IoHandler {
 private:
  struct PrivateTag {};

 public:
  explicit Dumper(PrivateTag) {}

  static std::shared_ptr<Dumper> Build() {
    return std::make_shared<Dumper>(PrivateTag{});
  }

  ResultOr<Outcome> HandleRead(io::FdHandle h,
                               const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kSuspend;
    constexpr size_t kSize = 1024;
    char buf[kSize];
    TRY_ASSIGN(auto n, NonBlockingRead(&o, fd, buf, kSize));
    if (n > 0) {
      Log(INFO) << "\n" << strings::Hexdump(buf, n);
    }
    return o;
  }

  ResultOr<Outcome> HandleWrite(io::FdHandle h,
                                const core::FileDescriptor& fd) override {
    return Outcome::kSuspend;
  }
};

Result Webdump() {
  TRY_ASSIGN(auto epoller, io::Epoller::Build(1));
  const uint16_t port = FLAG_LOOKUP(port);
  TRY_ASSIGN(
      auto listner,
      net::Listener::Build(
          epoller.get(), port,
          [e = epoller.get()](core::FileDescriptor fd, net::SocketAddress sa) {
            Log(INFO) << "Connection from: " << sa.ToString();
            auto h = Dumper::Build();
            CHECK_OK(e->SetNonBlockingAndRegister(std::move(h), std::move(fd)));
            //
          }));
  Log(INFO) << "Listener online on port [" << port << "].";
  SleepFor(base::Hours(1));
  Log(INFO) << "Listener timeout.";

  return Result::Ok();
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  core::util::DieElegantlyIfNotOk(Webdump());
  return EXIT_SUCCESS;
}
