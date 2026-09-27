#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

#include "core/vocabulary.h"
#include "net/socket_address.h"

namespace net {
namespace {

std::string FormatIp(const struct sockaddr_storage& s) {
  char buf[INET6_ADDRSTRLEN] = {0};
  if (s.ss_family == AF_INET) {
    const auto* sin = reinterpret_cast<const struct sockaddr_in*>(&s);
    ::inet_ntop(AF_INET, &(sin->sin_addr), buf, sizeof(buf));
  } else if (s.ss_family == AF_INET6) {
    const auto* sin6 = reinterpret_cast<const struct sockaddr_in6*>(&s);
    ::inet_ntop(AF_INET6, &(sin6->sin6_addr), buf, sizeof(buf));
  }
  return std::string(buf);
}

uint16_t ExtractPort(const struct sockaddr_storage& s) {
  if (s.ss_family == AF_INET) {
    const auto* sin = reinterpret_cast<const struct sockaddr_in*>(&s);
    return ntohs(sin->sin_port);
  } else if (s.ss_family == AF_INET6) {
    const auto* sin6 = reinterpret_cast<const struct sockaddr_in6*>(&s);
    return ntohs(sin6->sin6_port);
  }
  return 0;
}

}  // namespace

std::string SocketAddress::IpAddress() const { return FormatIp(ss_); }

uint16_t SocketAddress::Port() const { return ExtractPort(ss_); }

std::string SocketAddress::ToString() const {
  std::string ip = IpAddress();
  uint16_t p = Port();
  if (ss_.ss_family == AF_INET6) {
    return strings::Format("[{}]:{}", ip, p);
  }
  return strings::Format("{}:{}", ip, p);
}

}  // namespace net
