#include "core/idioms.h"
#include "core/syscalls.h"

namespace core::idioms {

Result ReadExactly(const FileDescriptor& fd, char* output, size_t bytes) {
  constexpr size_t kChunkSize = 1024 * 1024;
  size_t bytes_remain = bytes;
  while (bytes_remain > 0) {
    const size_t this_read = std::min<size_t>(bytes_remain, kChunkSize);
    TRY_ASSIGN(const ssize_t bytes_read, syscalls::Read(fd, output, this_read));
    bytes_remain -= bytes_read;
    output += bytes_read;
    if (bytes_read == 0) {
      return Result(Code::kExhausted);  // EOF
    }
  }
  return Result::Ok();
}

Result WriteExactly(const FileDescriptor& fd, std::string_view data) {
  size_t bytes_remain = data.length();
  const char* ptr = data.data();
  while (bytes_remain > 0) {
    TRY_ASSIGN(const ssize_t bytes_written,
               syscalls::Write(fd, ptr, bytes_remain));
    bytes_remain -= bytes_written;
    ptr += bytes_written;
    if (bytes_written == 0) {
      return Result(Code::kExhausted);  // Byte exhaustion, maybe? IDK.
    }
  }
  return Result::Ok();
}

}  // namespace core::idioms
