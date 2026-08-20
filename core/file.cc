#include "core/file.h"
#include "core/idioms.h"
#include "core/strings.h"
#include "core/syscalls.h"

namespace core {

Result WriteContentsToFile(std::string_view file_name,
                           std::string_view contents) {
  // Open file.
  TRY_ASSIGN(auto fd, syscalls::Open(file_name, O_WRONLY | O_CREAT, 0664));
  return idioms::WriteExactly(fd, contents);
}

ResultOr<std::string> ReadContentsFromFile(std::string_view file_name) {
  // Open file.
  TRY_ASSIGN(auto fd, syscalls::Open(file_name, O_RDONLY, 0));

  // Stat file and size the output buffer.
  constexpr size_t kCowardiceThreshold = 1024 * 1024;
  TRY_ASSIGN(const auto statbuf, syscalls::FStat(fd));
  const size_t file_size_bytes = statbuf.st_size;
  if (file_size_bytes > kCowardiceThreshold) {
    return Result(
        Code::kExhausted,
        strings::Format("File [{0}] has size [{1}] bytes, exceeding a "
                        "reasonable upper bound of [{2}] bytes.",
                        file_name, file_size_bytes, kCowardiceThreshold));
  }
  std::string ret(file_size_bytes, '\0');

  // Read the file.
  TRY(idioms::ReadExactly(fd, ret.data(), file_size_bytes));
  return ret;
}

}  // namespace core
