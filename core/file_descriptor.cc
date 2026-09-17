#include <unistd.h>

#include "base/rawlog.h"
#include "core/file_descriptor.h"

namespace core {
namespace {

constexpr int kInvalid = -1;

void CloseDoggedlyOrDie(int fd) {
  int ret = 0;
  while ((ret = ::close(fd)) < 0 && errno == EINTR) continue;
  RAW_CHECK(ret == 0) << "Close failed on FD " << fd;
}

}  // namespace

FileDescriptor::~FileDescriptor() {
  if (fd_ >= 0) {
    CloseDoggedlyOrDie(fd_);
  }
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(std::exchange(other.fd_, kInvalid)) {}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (this != &other) {
    if (fd_ >= 0) {
      CloseDoggedlyOrDie(fd_);
    }
    fd_ = std::exchange(other.fd_, kInvalid);
  }
  return *this;
}

ResultOr<int> FileDescriptor::Release() {
  if (fd_ >= 0) {
    return std::exchange(fd_, kInvalid);
  }

  return Result(Code::kPrecondition);
}

}  // namespace core
