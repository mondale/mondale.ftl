#ifndef CORE_SYSCALLS_H_
#define CORE_SYSCALLS_H_

#include <fcntl.h>
#include <sys/stat.h>

#include <string_view>

#include "core/file_descriptor.h"
#include "core/result.h"

namespace core::syscalls {

ResultOr<struct stat> FStat(const FileDescriptor& fd);
ResultOr<FileDescriptor> Open(std::string_view path, int flags, mode_t mode);
ResultOr<size_t> Read(const FileDescriptor& fd, char* buf, size_t count);
ResultOr<size_t> Write(const FileDescriptor& fd, const char* buf, size_t count);

}  // namespace core::syscalls

#endif  // #ifndef CORE_SYSCALLS_H_
