#include "core/file.h"
#include "core/idioms.h"
#include "core/strings.h"
#include "core/syscalls.h"

namespace core {

namespace {

constexpr size_t kCowardiceThreshold = 1024 * 1024;

ResultOr<std::string> FallbackReadContents(std::string_view file_name,
                                           const FileDescriptor& fd) {
  std::string ret;
  char buffer[4096];
  while (true) {
    TRY_ASSIGN(size_t bytes_read, syscalls::Read(fd, buffer, sizeof(buffer)));
    if (bytes_read == 0) {
      break;  // EOF reached
    }

    // Enforce cowardice threshold dynamically during streaming
    if (ret.size() + bytes_read > kCowardiceThreshold) {
      return Result(
          Code::kExhausted,
          strings::Format(
              "File [{0}] exceeded the reasonable upper bound of [{1}] bytes.",
              file_name, kCowardiceThreshold));
    }
    ret.append(buffer, bytes_read);
  }
  return ret;
}

}  // namespace

bool FileExists(std::string_view file_name) {
  return syscalls::Stat(file_name).IsOk();
}

bool FileIsReadable(std::string_view file_name) {
  return IsOk(syscalls::Access(file_name, R_OK));
}

bool FileIsWriteable(std::string_view file_name) {
  return !FileExists(file_name) || IsOk(syscalls::Access(file_name, W_OK));
}

Result WriteContentsToFile(std::string_view file_name,
                           std::string_view contents) {
  // Open file.
  TRY_ASSIGN(auto fd,
             syscalls::Open(file_name, O_WRONLY | O_CREAT | O_CLOEXEC, 0664));
  return idioms::WriteExactly(fd, contents);
}

ResultOr<std::string> ReadContentsFromFile(std::string_view file_name) {
  // Open file.
  auto fd_or = syscalls::Open(file_name, O_RDONLY | O_CLOEXEC, 0);
  if (!fd_or.IsOk()) {
    return Result(fd_or.result().code(),
                  strings::Format("File [{}] could not be opened.", file_name));
  }
  auto fd = std::move(fd_or).ValueOrDie();

  // Stat file and size the output buffer.
  TRY_ASSIGN(const auto statbuf, syscalls::FStat(fd));
  const size_t file_size_bytes = statbuf.st_size;
  if (file_size_bytes > kCowardiceThreshold) {
    return Result(
        Code::kExhausted,
        strings::Format("File [{0}] has size [{1}] bytes, exceeding a "
                        "reasonable upper bound of [{2}] bytes.",
                        file_name, file_size_bytes, kCowardiceThreshold));
  }

  if (file_size_bytes == 0) {
    return FallbackReadContents(file_name, fd);
  }

  // Read the file.
  std::string ret(file_size_bytes, '\0');
  TRY(idioms::ReadExactly(fd, ret.data(), file_size_bytes));
  return ret;
}

}  // namespace core
