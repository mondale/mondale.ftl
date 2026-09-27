#include <netinet/in.h>

#include "core/idioms.h"
#include "core/strings.h"
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

Result SetNonBlocking(const FileDescriptor& fd) {
  // TODO - need an idioms test
  TRY_ASSIGN(int flags, core::syscalls::Fcntl(fd, F_GETFL, 0));
  TRY_ASSIGN(flags, core::syscalls::Fcntl(fd, F_SETFL, flags | O_NONBLOCK));
  return Result::Ok();
}

ResultOr<FileDescriptor> NewListenSocket(uint16_t port, int backlog) {
  if (backlog <= 0) {
    return core::InvalidArgumentError(
        strings::Format("Listen backlog [{}] must be positive.", backlog));
  }

  TRY_ASSIGN(auto fd, core::syscalls::Socket(AF_INET6, SOCK_STREAM, 0));
  TRY(SetNonBlocking(fd));

  // Allow the socket to accept v4 or v6 by clearing the v6 only flag.
  int opt = 0;  // v6 or v4
  TRY(core::syscalls::SetSockOpt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt,
                                 sizeof(opt)));

  // Bind to the ipv6 any address.
  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;
  addr.sin6_port = htons(port);
  TRY(core::syscalls::Bind(fd, reinterpret_cast<const struct sockaddr*>(&addr),
                           sizeof(addr)));

  // Begin listening.
  TRY(core::syscalls::Listen(fd, backlog));
  return fd;
}

}  // namespace core::idioms
