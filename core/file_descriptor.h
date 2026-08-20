#ifndef CORE_FILE_DESCRIPTOR_H_
#define CORE_FILE_DESCRIPTOR_H_

#include "core/result.h"

namespace core {

// Holds an open file descriptor and closes on dtor.
class FileDescriptor {
 public:
  explicit FileDescriptor(int fd) : fd_(fd) {}
  ~FileDescriptor();

  // Delete Copy Operations
  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;

  // Default Move Operations
  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;

  const int fd() const { return fd_; }

  // Ownership of fd passes to caller.
  // Returns precondition error if the FD is already released.
  ResultOr<int> Release();

 private:
  int fd_;
};

}  // namespace core

#endif  // #ifndef CORE_FILE_DESCRIPTOR_H_
