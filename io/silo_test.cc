#include <string.h>

#include "core/idioms.h"
#include "core/syscalls.h"
#include "io/context.h"
#include "io/io_handler.h"
#include "io/silo.h"
#include "testing/testing.h"

namespace io {

struct Stuff final {
  int64_t Reads() const { return read_upcalls.load(std::memory_order_acquire); }
  int64_t Writes() const {
    return write_upcalls.load(std::memory_order_acquire);
  }
  int64_t BytesRead() const {
    return bytes_read.load(std::memory_order_acquire);
  }
  int64_t BytesWritten() const {
    return bytes_written.load(std::memory_order_acquire);
  }

  std::atomic<int> refs{0};
  std::atomic<int64_t> read_upcalls{0};
  std::atomic<int64_t> write_upcalls{0};
  std::atomic<int64_t> bytes_read{0};
  std::atomic<int64_t> bytes_written{0};
};

class HandlerBase {
 public:
  explicit HandlerBase(Stuff* s) : s_(s) { s_->refs++; }
  ~HandlerBase() {
    s_->refs--;
    CHECK_GE(s_->refs.load(std::memory_order_acquire), 0);
  }

  void HandledWrite(size_t bytes) {
    s_->write_upcalls++;
    s_->bytes_written += bytes;
  }

  void HandledRead(size_t bytes) {
    s_->read_upcalls++;
    s_->bytes_read += bytes;
  }

 private:
  Stuff* const s_;
};

// Always willing to read, down for a good time.
class EagerSwallowHandler final : public HandlerBase, public IoHandler {
 public:
  explicit EagerSwallowHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  virtual ~EagerSwallowHandler() {}
  static constexpr size_t kSwallowSize = 64;

  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kYield;
    TRY_ASSIGN(const auto bytes, NonBlockingRead(&o, fd, buf_, kSwallowSize));
    HandledRead(bytes);
    return o;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    // I never want to write.
    HandledWrite(0);
    return Outcome::kSuspend;
  }

  char buf_[kSwallowSize];
};

// Always has something to write.
class GarbageFountainHandler final : public HandlerBase, public IoHandler {
 public:
  explicit GarbageFountainHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    // I never want to read.
    HandledRead(0);
    return Outcome::kSuspend;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    Outcome o = Outcome::kYield;
    constexpr const char kMsg[] =
        "Passersby were amazed by unusually large amounts of blood. ";
    TRY_ASSIGN(const auto bytes, NonBlockingWrite(&o, fd, kMsg, strlen(kMsg)));
    HandledWrite(bytes);
    return o;
  }
};

// Hates all file descriptors and just wants them to die.
class ClosingHandler final : public HandlerBase, public IoHandler {
 public:
  explicit ClosingHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    // Eeew! Close this thing!
    HandledRead(0);
    return Outcome::kClose;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    // Eeew! Close this thing!
    HandledWrite(0);
    return Outcome::kClose;
  }
};

// Merrily echoes whatever it reads, but has a small internal buffer.
class EchoingHandler final : public HandlerBase, public IoHandler {
 public:
  explicit EchoingHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  static constexpr size_t kSize = 64;
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    // Call me back when my buffer is empty.
    if (!buffer_.empty()) {
      HandledRead(0);
      return Outcome::kSuspend;
    }

    buffer_.resize(kSize);
    Outcome o = Outcome::kYield;
    TRY_ASSIGN(auto bytes, NonBlockingRead(&o, fd, &buffer_[0], kSize));
    HandledRead(bytes);
    CHECK_LE(bytes, kSize);
    buffer_.resize(bytes);
    c->RequestWrite(h);
    c->RequestWrite(h);  // fun to do it twice!
    return o;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    // Call me back when my buffer is not empty.
    if (buffer_.empty()) {
      HandledWrite(0);
      return Outcome::kSuspend;
    }

    Outcome o = Outcome::kYield;
    auto remain = buffer_.size();
    TRY_ASSIGN(auto bytes, NonBlockingWrite(&o, fd, &buffer_[0], remain));
    HandledWrite(bytes);
    CHECK_LE(bytes, remain);
    if (bytes < remain) {
      memmove(&buffer_[0], &buffer_[bytes], (remain - bytes));
    }
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
    // Defaults preclude load shedding.
    utils_[0] = 99;
    utils_[1] = 99;
    utils_[2] = 99;
    utils_[3] = 99;
  }

  ~SiloTest() override {
    exiting_.Notify();
    Poke();
    thread_.reset();

    esh_.reset();
    gfh_.reset();
    ch_.reset();
    eh_.reset();
    CHECK_EQ(expected_living_handlers_,
             stuff_.refs.load(std::memory_order_acquire));
  }

  void ShedFn(internal::HFDs&& i) {
    MutexLock lock(&mu_);
    sheds_.emplace_back(std::move(i));
  }

  void PeekFn(std::vector<int>* us) {
    us->resize(4);
    MutexLock lock(&mu_);
    (*us)[0] = utils_[0];
    (*us)[1] = util_.load(std::memory_order_acquire);
    (*us)[2] = utils_[2];
    (*us)[3] = utils_[3];
  }

  std::pair<core::FileDescriptor, core::FileDescriptor> MakeSocketPair() {
    auto pair = std::move(
        core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie());
    CHECK_OK(core::idioms::SetNonBlocking(pair.second));
    return pair;
  }

  void Poke() { CHECK_OK(core::syscalls::EventFdWrite(event_fd_, 1)); }

  core::FileDescriptor InstallPipe(std::shared_ptr<IoHandler> h) {
    auto [mine, silos] = MakeSocketPair();
    internal::HFDs i;
    i.h = h;
    i.fds.push_back(std::move(silos));
    h->SetAffinity(1);
    auto poke = MakeCleanup([this]() { Poke(); });
    MutexLock l(&inbound_mu_);
    inbound_.emplace_back(std::move(i));
    return std::move(mine);
  }

  void Write(const core::FileDescriptor& fd, std::string_view data) {
    CHECK_OK(core::idioms::WriteExactly(fd, data));
  }

  bool Await(std::move_only_function<bool()> cond) {
    constexpr Duration kMaxWait = Seconds(1);
    const auto stop = WallTime::Now() + kMaxWait;
    while (WallTime::Now() < stop) {
      if (cond()) return true;
      SleepFor(Milliseconds(1));
    }
    return false;
  }

  core::FileDescriptor event_fd_ =
      core::syscalls::EventFd(0, EFD_CLOEXEC | EFD_NONBLOCK).ValueOrDie();
  Notification exiting_;
  Mutex inbound_mu_;
  Silo::InList inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int> util_;

  Mutex mu_;
  std::list<internal::HFDs> sheds_ GUARDED_BY(mu_);
  std::vector<int> utils_ GUARDED_BY(mu_);

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
  Stuff stuff_;
  std::shared_ptr<IoHandler> esh_ =
      std::make_shared<EagerSwallowHandler>(&stuff_);
  std::shared_ptr<IoHandler> gfh_ =
      std::make_shared<GarbageFountainHandler>(&stuff_);
  std::shared_ptr<IoHandler> ch_ = std::make_shared<ClosingHandler>(&stuff_);
  std::shared_ptr<IoHandler> eh_ = std::make_shared<EchoingHandler>(&stuff_);
};

TEST_F(SiloTest, SetupAndTeardownWorks) {}

TEST_F(SiloTest, PokeTolerant) {
  // Just jab the silo in the eye repeatedly and it shouldn't break.
  for (int i = 0; i < 38; ++i) {
    for (int j = 0; j < i; ++j) Poke();
    SleepFor(Microseconds(100));
  }
}

TEST_F(SiloTest, EagerSwallowCanReadBasic) {
  auto fd = InstallPipe(esh_);
  std::string_view msg = "Hello, there!";
  Write(fd, msg);
  EXPECT_TRUE(Await([&]() { return stuff_.BytesRead() >= msg.length(); }));
  EXPECT_EQ(stuff_.BytesRead(), msg.length());
  EXPECT_GE(stuff_.Reads(), 2);   // Yield, Eagain
  EXPECT_EQ(stuff_.Writes(), 1);  // Suspend
}

TEST_F(SiloTest, EagerSwallowCanReallySwallow) {
  constexpr int kIters = 100000;
  auto fd = InstallPipe(esh_);
  std::string_view msg = "0123456789";
  for (int i = 0; i < kIters; ++i) {
    Write(fd, msg);
  }
  const auto size = msg.length() * kIters;
  EXPECT_TRUE(Await([&]() { return stuff_.BytesRead() >= size; }));
  EXPECT_EQ(stuff_.BytesRead(), size);
  const auto min_reads = size / EagerSwallowHandler::kSwallowSize;
  EXPECT_GE(stuff_.Reads(), min_reads + 1);  // Yields, Eagain at least once
  EXPECT_EQ(stuff_.Writes(), 1);             // Suspend
}

TEST_F(SiloTest, GarbageIsAlwaysReadable) {
  auto fd = InstallPipe(gfh_);
  char buf[128];
  memset(buf, 0, sizeof(buf));
  // Read from the test fixture end; garbage fountain writes continuously.
  EXPECT_TRUE(Await([&]() {
    auto r = core::syscalls::Read(fd, buf, 128);
    return r.IsOk() && r.ValueOrDie() > 0;
  }));
  EXPECT_GT(stuff_.Writes(), 0);
}

TEST_F(SiloTest, CloserKillsTheFd) {
  // Test case that exercises the case of a voluntary close via the
  // CloserHandler.
  expected_living_handlers_ = 0;
  auto fd = InstallPipe(ch_);

  // The CloserHandler returns Outcome::kClose immediately, causing the silo
  // to close its end of the socketpair. We wait for our end (fd) to register
  // EOF (read returning 0) or a connection error.
  char buf[16];
  EXPECT_TRUE(Await([&]() {
    auto r = core::syscalls::Read(fd, buf, sizeof(buf));
    if (r.IsOk()) {
      // 0 bytes read indicates EOF (the peer/silo closed the socket).
      return r.ValueOrDie() == 0;
    }
    // Any socket error (e.g., ECONNRESET, EPIPE) also confirms the FD is dead.
    return !r.result().Is(Code::kEagain);
  }));
}

TEST_F(SiloTest, Echo) {
  // Test case using the echoing handler to push a hundred thousand bytes in
  // and read them back precisely.
  auto fd = InstallPipe(eh_);
  constexpr size_t kTotalBytes = 100000;
  std::string outbound(kTotalBytes, 'z');

  base::CreateDetachedThread("WriteBuddy", [&]() { Write(fd, outbound); });

  std::string inbound;
  inbound.resize(kTotalBytes);
  size_t total_read = 0;

  // Read back the echoed bytes from the same socket pair.
  EXPECT_TRUE(Await([&]() {
    char buf[4096];
    auto r = core::syscalls::Read(fd, buf, sizeof(buf));
    if (r.IsOk() && r.ValueOrDie() > 0) {
      size_t n = r.ValueOrDie();
      memcpy(&inbound[total_read], buf, n);
      total_read += n;
    }
    return total_read >= kTotalBytes;
  }));

  EXPECT_EQ(inbound, outbound);
}

TEST_F(SiloTest, LoadShedding) {
  // Advertise low utilization in other silos to encourage load shedding.
  {
    MutexLock l(&mu_);
    utils_[0] = utils_[2] = utils_[3] = 3;
  }

  // Insert a couple pipes, advertise low utilzition in other silos, and then
  // observe some ejected FDs.
  auto fd1 = InstallPipe(gfh_);
  auto fd2 = InstallPipe(gfh_);

  // Have the silo also read its own handiwork, inefficiently.
  {
    internal::HFDs i;
    i.h = esh_;
    i.fds.push_back(std::move(fd1));
    i.fds.push_back(std::move(fd2));
    esh_->SetAffinity(1);
    auto poke = MakeCleanup([this]() { Poke(); });
    MutexLock l(&inbound_mu_);
    inbound_.emplace_back(std::move(i));
  }

  // Verify that at least one file descriptor/handler pair was ejected to
  // sheds_.
  EXPECT_TRUE(Await([&]() {
    MutexLock l(&mu_);
    return !sheds_.empty();
  }));
  MutexLock l(&mu_);
  EXPECT_GE(sheds_.size(), 1);
  sheds_.clear();
}

}  // namespace io
