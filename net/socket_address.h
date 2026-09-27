#ifndef NET_SOCKET_ADDRESS_H_
#define NET_SOCKET_ADDRESS_H_

#include <sys/socket.h>

#include <cstdint>
#include <string>

namespace net {

// Convenience wrapper around a sockaddr_storage.
class SocketAddress final {
 public:
  explicit SocketAddress(const struct sockaddr_storage& ss) {
    memcpy(&ss_, &ss, sizeof(ss_));
  }

  SocketAddress(const SocketAddress&) = default;
  SocketAddress& operator=(const SocketAddress&) = default;
  SocketAddress(SocketAddress&&) = default;
  SocketAddress& operator=(SocketAddress&&) = default;

  ~SocketAddress() = default;

  // Returns ip:port, e.g., "1.2.3.4:5678" or "[2001:db8::1]:5678" for IPv6.
  std::string ToString() const;

  // Returns ip, e.g., "1.2.3.4" or a v6 version of same.
  std::string IpAddress() const;

  // Returns port number, e.g., 5678.
  uint16_t Port() const;

 private:
  struct sockaddr_storage ss_;
};

}  // namespace net

#endif  // #ifndef NET_SOCKET_ADDRESS_H_
