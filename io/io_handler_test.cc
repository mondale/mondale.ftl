#include <string.h>

#include "core/idioms.h"
#include "core/syscalls.h"
#include "io/io_handler.h"
#include "testing/testing.h"

namespace io {

class TestIoHandler final : public IoHandler {
 public:
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }

  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override {
    return Outcome::kFdEagain;
  }
};

TEST(Affininty) {
  TestIoHandler tih;
  tih.SetAffinity(4);
  EXPECT_EQ(tih.GetAffinity(), 4);
}

TEST(IoHandlerHelpers_NonBlockingWriteSuccess) {
  auto pair = std::move(
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie());
  CHECK_OK(core::idioms::SetNonBlocking(pair.first));
  CHECK_OK(core::idioms::SetNonBlocking(pair.second));

  TestIoHandler tih;
  IoHandler::Outcome outcome = IoHandler::Outcome::kClose;

  const char* msg = "mondale-io";
  auto write_res = tih.NonBlockingWrite(&outcome, pair.first, msg, strlen(msg));
  EXPECT_TRUE(write_res.IsOk());
  EXPECT_EQ(write_res.ValueOrDie(), strlen(msg));
  EXPECT_EQ(outcome, IoHandler::Outcome::kYield);
}

TEST(IoHandlerHelpers_NonBlockingReadSuccess) {
  auto pair = std::move(
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie());
  CHECK_OK(core::idioms::SetNonBlocking(pair.first));
  CHECK_OK(core::idioms::SetNonBlocking(pair.second));

  // Prime the socket with data
  const char* msg = "mondale-io";
  CHECK_OK(core::idioms::WriteExactly(pair.first, msg));

  TestIoHandler tih;
  IoHandler::Outcome outcome = IoHandler::Outcome::kClose;

  char buf[64];
  memset(buf, 0, sizeof(buf));
  auto read_res = tih.NonBlockingRead(&outcome, pair.second, buf, sizeof(buf));
  EXPECT_TRUE(read_res.IsOk());
  EXPECT_EQ(read_res.ValueOrDie(), strlen(msg));
  EXPECT_EQ(0, memcmp(buf, msg, strlen(msg)));
  EXPECT_EQ(outcome, IoHandler::Outcome::kYield);
}

TEST(IoHandlerHelpers_NonBlockingReadEagain) {
  auto pair = std::move(
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie());
  CHECK_OK(core::idioms::SetNonBlocking(pair.second));

  TestIoHandler tih;
  IoHandler::Outcome outcome = IoHandler::Outcome::kYield;

  // Read from an empty non-blocking socket should yield kFdEagain
  char buf[64];
  auto read_res = tih.NonBlockingRead(&outcome, pair.second, buf, sizeof(buf));
  EXPECT_TRUE(read_res.IsOk());
  EXPECT_EQ(read_res.ValueOrDie(), 0);
  EXPECT_EQ(outcome, IoHandler::Outcome::kFdEagain);
}

TEST(IoHandlerHelpers_NonBlockingReadEof) {
  auto pair = std::move(
      core::syscalls::SocketPair(AF_UNIX, SOCK_STREAM, 0).ValueOrDie());
  CHECK_OK(core::idioms::SetNonBlocking(pair.second));

  // Close the writing end immediately to trigger EOF on the reader
  pair.first = core::FileDescriptor();

  TestIoHandler tih;
  IoHandler::Outcome outcome = IoHandler::Outcome::kYield;

  char buf[64];
  auto read_res = tih.NonBlockingRead(&outcome, pair.second, buf, sizeof(buf));
  EXPECT_EQ(outcome, IoHandler::Outcome::kClose);
}

}  // namespace io
