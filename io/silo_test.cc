#include "core/syscalls.h"
#include "io/silo.h"
#include "testing/testing.h"

namespace io {

class FakeHandler final : public IoHandler {
  Outcome HandleRead(Context* c, FdHandle h,
                     const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }
  Outcome HandleWrite(Context* c, FdHandle h,
                      const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }
};

class SiloTest : public ::testing::Test {
 protected:
  SiloTest() {
    utils_.resize(4);
    utils_[0] = 0;
    utils_[1] = 0;
    utils_[2] = 0;
    utils_[3] = 0;
  }

  ~SiloTest() override {
    exiting_.Notify();
    static_cast<void>(std::move(thread_));
  }

  void ShedFn(internal::HFDs&& i) { sheds_.emplace_back(std::move(i)); }

  void PeekFn(std::vector<int>* us) {
    us->resize(4);
    (*us)[0] = utils_[0];
    (*us)[1] = util_.load(std::memory_order_acquire);
    (*us)[2] = utils_[2];
    (*us)[3] = utils_[3];
  }

  core::FileDescriptor event_fd_ =
      core::syscalls::EventFd(0, EFD_CLOEXEC | EFD_NONBLOCK).ValueOrDie();
  Notification exiting_;
  Mutex inbound_mu_;
  Silo::InList inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int> util_;

  std::list<internal::HFDs> sheds_;
  std::vector<int> utils_;

  Silo silo_{1,
             &event_fd_,
             &exiting_,
             &inbound_mu_,
             &inbound_,
             &util_,
             [this](internal::HFDs&& i) { ShedFn(std::move(i)); },
             [this](std::vector<int>* utils) { PeekFn(utils); }};

  std::unique_ptr<Thread> thread_ =
      CreateThread("SiloTest", [this]() { silo_.ThreadMain(); });
};

TEST(SetupAndTeardownWorks) {}

}  // namespace io
