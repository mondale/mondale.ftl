#include <string.h>

#include "io/context.h"
#include "io/test_handlers.h"

namespace io::testing {

ResultOr<io::IoHandler::Outcome> EagerSwallowHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kYield;
  TRY_ASSIGN(const auto bytes, NonBlockingRead(&o, fd, buf_, kSwallowSize));
  HandledRead(bytes);
  return o;
}

ResultOr<io::IoHandler::Outcome> EagerSwallowHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  // I never want to write.
  HandledWrite(0);
  return Outcome::kSuspend;
}

ResultOr<io::IoHandler::Outcome> GarbageFountainHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  // I never want to read.
  HandledRead(0);
  return Outcome::kSuspend;
}

ResultOr<io::IoHandler::Outcome> GarbageFountainHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  Outcome o = Outcome::kYield;
  constexpr const char kMsg[] =
      "Passersby were amazed by unusually large amounts of blood. ";
  TRY_ASSIGN(const auto bytes, NonBlockingWrite(&o, fd, kMsg, strlen(kMsg)));
  HandledWrite(bytes);
  return o;
}

ResultOr<io::IoHandler::Outcome> ClosingHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  // Eeew! Close this thing!
  HandledRead(0);
  return Outcome::kClose;
}

ResultOr<io::IoHandler::Outcome> ClosingHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  // Eeew! Close this thing!
  HandledWrite(0);
  return Outcome::kClose;
}

ResultOr<io::IoHandler::Outcome> EchoingHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
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

ResultOr<io::IoHandler::Outcome> EchoingHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
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

ResultOr<io::IoHandler::Outcome> CarlyHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  // Hey, I just met you!
  handle_ = h;
  Outcome o = Outcome::kSuspend;
  if (maybe_) {
    // And this is crazy!
    maybe_ = false;
    char buf[64];
    TRY_ASSIGN(const auto bytes, NonBlockingRead(&o, fd, buf, 64));
    HandledRead(bytes);
    return o;
  }

  HandledRead(0);
  // But here's my number, so call me maybe!
  c->Run(h, [&](Context* c) { CallMeMaybe(c); });
  return o;
}

ResultOr<io::IoHandler::Outcome> CarlyHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  handle_ = h;
  HandledWrite(0);
  return Outcome::kSuspend;
}

void CarlyHandler::CallMeMaybe(Context* c) {
  maybe_ = true;
  c->RequestRead(handle_);
}

ResultOr<io::IoHandler::Outcome> SquattingHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  HandledRead(0);
  return Outcome::kSuspend;
}

ResultOr<io::IoHandler::Outcome> SquattingHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  HandledWrite(0);
  return Outcome::kSuspend;
}

Result SquattingHandler::HandleIdle(Context* c, FdHandle h,
                                    const core::FileDescriptor& fd) {
  idles_++;
  return Result::Ok();
}

ResultOr<io::IoHandler::Outcome> RapidIdleHandler::HandleRead(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  HandledRead(0);
  return Outcome::kSuspend;
}

ResultOr<io::IoHandler::Outcome> RapidIdleHandler::HandleWrite(
    Context* c, FdHandle h, const core::FileDescriptor& fd) {
  HandledWrite(0);
  return Outcome::kSuspend;
}

}  // namespace io::testing
