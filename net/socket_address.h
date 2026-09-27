#ifndef NET_SOCKET_ADDRESS_H_
#define NET_SOCKET_ADDRESS_H_

#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "core/vocabulary.h"

namespace net {

// Convenience wrapper around a sockaddr_storage.
class SocketAddress final {
 public:
  SocketAddress() { memset(&ss_, 0, sizeof(ss_)); }
  explicit SocketAddress(const struct sockaddr_storage& ss) {
    memcpy(&ss_, &ss, sizeof(ss_));
  }

  SocketAddress(const SocketAddress&) = default;
  SocketAddress& operator=(const SocketAddress&) = default;
  SocketAddress(SocketAddress&&) = default;
  SocketAddress& operator=(SocketAddress&&) = default;

  ~SocketAddress() = default;

  // Parses an address string of the form "ip:port" (e.g., "1.2.3.4:5678" or
  // "[2001:db8::1]:5678").
  static ResultOr<SocketAddress> FromString(std::string_view addr);

  // Returns ip:port, e.g., "1.2.3.4:5678" or "[2001:db8::1]:5678" for IPv6.
  std::string ToString() const;

  // Returns ip, e.g., "1.2.3.4" or a v6 version of same.
  std::string IpAddress() const;

  // Returns port number, e.g., 5678.
  uint16_t Port() const;

  struct sockaddr* AsSockaddr() {
    return reinterpret_cast<struct sockaddr*>(&ss_);
  }

  socklen_t socklen() const { return sizeof(ss_); }

 private:
  struct sockaddr_storage ss_;
};

}  // namespace net

#endif  // #ifndef NET_SOCKET_ADDRESS_H_
