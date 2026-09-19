#include <string.h>

#include "core/idioms.h"
#include "core/syscalls.h"
#include "io/context.h"
#include "io/io_handler.h"
#include "io/silo.h"
#include "testing/testing.h"

namespace io {

// TODO - move these to helpers.
// TODO - sometimes these should say kClose, figure out when.

ResultOr<size_t> NonBlockingRead(IoHandler::Outcome* outcome,
                                 const core::FileDescriptor& fd, char* buf,
                                 size_t count) {
  auto r = core::syscalls::Read(fd, buf, count);
  if (r.IsOk()) {
    const auto bytes_read = r.ValueOrDie();
    if (0 == bytes_read) {
      // EOF -> close the FD.
      *outcome = IoHandler::Outcome::kClose;
      return 0;
    }

    // Could be a full or partial read, either way, yield and call again.
    *outcome = IoHandler::Outcome::kYield;
    return bytes_read;
  } else if (r.result().Is(Code::kEagain)) {
    // No more bytes, convert EAGAIN to an ok status.
    *outcome = IoHandler::Outcome::kFdEagain;
    return 0;
  }

  // Some errors here should become OK.
  *outcome = IoHandler::Outcome::kClose;
  return r.result();
}

ResultOr<size_t> NonBlockingWrite(IoHandler::Outcome* outcome,
                                  const core::FileDescriptor& fd,
                                  const char* buf, size_t count) {
  auto r = core::syscalls::Write(fd, buf, count);
  if (r.IsOk()) {
    // Presumably more bytes are writeable, so call again.
    *outcome = IoHandler::Outcome::kYield;
    return r.ValueOrDie();
  } else if (r.result().Is(Code::kEagain)) {
    // Backpressure, convert EAGAIN to an ok status.
    *outcome = IoHandler::Outcome::kFdEagain;
    return 0;
  }

  // Some errors here should become OK.
  *outcome = IoHandler::Outcome::kClose;
  return r.result();
}

class Refs {
 public:
  Refs(int* r) : r_(r) { (*r_)++; }
  ~Refs() {
    (*r_)--;
    CHECK_GE(*r_, 0);
  }

 private:
  int* const r_;
};

// Always willing to read, down for a good time.
class EagerSwallowHandler final : public Refs, public IoHandler {
 public:
  explicit EagerSwallowHandler(int* r) : Refs(r), IoHandler() {}
  static constexpr size_t kSwallowSize = 64;
  Outcome HandleRead(Context* c, FdHandle h,
                     const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kYield;
    CHECK_OK(NonBlockingRead(&o, fd, buf_, kSwallowSize).result());
    return o;
  }

  Outcome HandleWrite(Context* c, FdHandle h,
                      const core::FileDescriptor& fd) override {
    // I never want to write.
    return Outcome::kSuspend;
  }

  char buf_[kSwallowSize];
};

// Always has something to write.
class GarbageFountainHandler final : public Refs, public IoHandler {
 public:
  explicit GarbageFountainHandler(int* r) : Refs(r), IoHandler() {}
  Outcome HandleRead(Context* c, FdHandle h,
                     const core::FileDescriptor& fd) override {
    // I never want to read.
    return Outcome::kSuspend;
  }

  Outcome HandleWrite(Context* c, FdHandle h,
                      const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kYield;
    constexpr const char kMsg[] =
        "Passersby were amazed by unusually large amounts of blood. ";
    CHECK_OK(NonBlockingWrite(&o, fd, kMsg, strlen(kMsg)).result());
    return o;
  }
};

// Hates all file descriptors and just wants them to die.
class ClosingHandler final : public Refs, public IoHandler {
 public:
  explicit ClosingHandler(int* r) : Refs(r), IoHandler() {}
  Outcome HandleRead(Context* c, FdHandle h,
                     const core::FileDescriptor& fd) override {
    // Eeew! Close this thing!
    return Outcome::kClose;
  }

  Outcome HandleWrite(Context* c, FdHandle h,
                      const core::FileDescriptor& fd) override {
    // Eeew! Close this thing!
    return Outcome::kClose;
  }
};

// Merrily echoes whatever it reads, but has a small internal buffer.
class EchoingHandler final : public Refs, public IoHandler {
 public:
  explicit EchoingHandler(int* r) : Refs(r), IoHandler() {}
  static constexpr size_t kSize = 64;
  Outcome HandleRead(Context* c, FdHandle h,
                     const core::FileDescriptor& fd) override {
    // Call me back when my buffer is empty.
    if (!buffer_.empty()) return Outcome::kSuspend;
    buffer_.resize(kSize);
    Outcome o = Outcome::kYield;
    auto bytes = NonBlockingRead(&o, fd, &buffer_[0], kSize).ValueOrDie();
    CHECK_LE(bytes, kSize);
    buffer_.resize(bytes);
    c->RequestWrite(h);
    c->RequestWrite(h);  // fun to do it twice!
    return o;
  }

  Outcome HandleWrite(Context* c, FdHandle h,
                      const core::FileDescriptor& fd) override {
    // Call me back when my buffer is not empty.
    if (buffer_.empty()) return Outcome::kSuspend;

    Outcome o = Outcome::kYield;
    auto remain = buffer_.size();
    auto bytes = NonBlockingWrite(&o, fd, &buffer_[0], remain).ValueOrDie();
    CHECK_LE(bytes, remain);
    memmove(&buffer_[0], &buffer_[bytes], (remain - bytes));
    buffer_.resize(remain - bytes);

    // Deliberately call more than once because Silo can just DEAL WITH IT.
    if (buffer_.empty()) {
      c->RequestRead(h);
      c->RequestRead(h);
    }
    return o;
  }

  std::vector<char> buffer_;
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
    thread_.reset();

    esh_.reset();
    gfh_.reset();
    ch_.reset();
    eh_.reset();
    CHECK_EQ(expected_living_handlers_, living_handlers_);
  }

  void ShedFn(internal::HFDs&& i) { sheds_.emplace_back(std::move(i)); }

  void PeekFn(std::vector<int>* us) {
    us->resize(4);
    (*us)[0] = utils_[0];
    (*us)[1] = util_.load(std::memory_order_acquire);
    (*us)[2] = utils_[2];
    (*us)[3] = utils_[3];
  }

  std::pair<core::FileDescriptor, core::FileDescriptor> MakePipe() {
    auto pair = std::move(core::syscalls::Pipe2(FD_CLOEXEC).ValueOrDie());
    CHECK_OK(core::idioms::SetNonBlocking(pair.second));
    return pair;
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

  // Change this if you expect Silo to still be retaining references at the end
  // of test.
  int expected_living_handlers_ = 0;
  int living_handlers_ = 0;
  std::shared_ptr<IoHandler> esh_ =
      std::make_shared<EagerSwallowHandler>(&living_handlers_);
  std::shared_ptr<IoHandler> gfh_ =
      std::make_shared<GarbageFountainHandler>(&living_handlers_);
  std::shared_ptr<IoHandler> ch_ =
      std::make_shared<ClosingHandler>(&living_handlers_);
  std::shared_ptr<IoHandler> eh_ =
      std::make_shared<EchoingHandler>(&living_handlers_);
};

TEST_F(SiloTest, SetupAndTeardownWorks) {}

}  // namespace io
