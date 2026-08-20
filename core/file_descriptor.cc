#include <unistd.h>

#include "base/rawlog.h"
#include "core/file_descriptor.h"

namespace core {
namespace {

constexpr int kInvalid = -1;

}  // namespace

FileDescriptor::~FileDescriptor() {
  if (fd_ >= 0) {
    int ret = 0;
    while (::close(fd_) < 0 && errno == EINTR) continue;
    RAW_CHECK(ret == 0) << "Close failed on FD " << fd_;
  }
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept {
  if (&other != this) {
    fd_ = other.fd_;
    other.fd_ = kInvalid;
  }
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (&other != this) {
    fd_ = other.fd_;
    other.fd_ = kInvalid;
  }
  return *this;
}

ResultOr<int> FileDescriptor::Release() {
  if (fd_ >= 0) {
    const int ret = fd_;
    fd_ = -kInvalid;
    return ret;
  }

  return Result(Code::kPrecondition);
}

}  // namespace core
