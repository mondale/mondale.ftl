#ifndef CORE_SYSCALLS_H_
#define CORE_SYSCALLS_H_

#include <fcntl.h>

#include <string_view>

#include "core/file_descriptor.h"
#include "core/result.h"

namespace core::syscalls {

ResultOr<FileDescriptor> Open(std::string_view path, int flags, mode_t mode);

}  // namespace core::syscalls

#endif  // #ifndef CORE_SYSCALLS_H_
