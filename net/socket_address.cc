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

// static
ResultOr<SocketAddress> SocketAddress::FromString(std::string_view addr_str) {
  std::string ip_part;
  uint16_t port_val = 0;

  // Check if it's an IPv6 bracketed format like "[::1]:8080"
  if (!addr_str.empty() && addr_str.front() == '[') {
    size_t close_bracket = addr_str.find(']');
    if (close_bracket == std::string_view::npos) {
      return core::InvalidArgumentError(
          "Missing closing bracket for IPv6 address");
    }
    ip_part = std::string(addr_str.substr(1, close_bracket - 1));

    if (close_bracket + 1 >= addr_str.size() ||
        addr_str[close_bracket + 1] != ':') {
      return core::InvalidArgumentError(
          "Missing port separator after IPv6 address");
    }
    std::string_view port_str = addr_str.substr(close_bracket + 2);
    TRY_ASSIGN(int parsed_port, strings::ParseAs<int>(port_str));
    if (parsed_port < 0 || parsed_port > 65535) {
      return core::InvalidArgumentError("Port out of range");
    }
    port_val = static_cast<uint16_t>(parsed_port);
  } else {
    // Look for the last colon to separate IP from port (handles IPv4 like
    // "1.2.3.4:80")
    size_t last_colon = addr_str.rfind(':');
    if (last_colon == std::string_view::npos) {
      return core::InvalidArgumentError(
          "Missing port in socket address string");
    }

    // Check if there are multiple colons (unbracketed IPv6)
    size_t first_colon = addr_str.find(':');
    if (first_colon != last_colon) {
      // Multiple colons without brackets is ambiguous/unsupported
      return core::InvalidArgumentError(
          "Unbracketed IPv6 addresses must use brackets [ip]:port");
    }

    ip_part = std::string(addr_str.substr(0, last_colon));
    std::string_view port_str = addr_str.substr(last_colon + 1);
    TRY_ASSIGN(int parsed_port, strings::ParseAs<int>(port_str));
    if (parsed_port < 0 || parsed_port > 65535) {
      return core::InvalidArgumentError("Port out of range");
    }
    port_val = static_cast<uint16_t>(parsed_port);
  }

  struct sockaddr_storage ss;
  std::memset(&ss, 0, sizeof(ss));

  // Try parsing as IPv4 first
  struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(&ss);
  if (::inet_pton(AF_INET, ip_part.c_str(), &(sin->sin_addr)) == 1) {
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port_val);
    return SocketAddress(ss);
  }

  // Try parsing as IPv6
  struct sockaddr_in6* sin6 = reinterpret_cast<struct sockaddr_in6*>(&ss);
  if (::inet_pton(AF_INET6, ip_part.c_str(), &(sin6->sin6_addr)) == 1) {
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(port_val);
    return SocketAddress(ss);
  }

  return core::InvalidArgumentError(
      strings::Format("Invalid IP address: %s", ip_part.c_str()));
}

}  // namespace net
