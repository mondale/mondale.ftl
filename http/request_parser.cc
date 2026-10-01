#include <algorithm>
#include <cctype>
#include <string_view>

#include "http/request_parser.h"

using core::StreamFatalError;

namespace http {
namespace {

// Case-insensitive string view comparator or helper for headers if needed.
bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i]))) {
      return false;
    }
  }
  return true;
}

bool HasBody(RequestParser::Method m) {
  switch (m) {
    case RequestParser::Method::kPost:
    case RequestParser::Method::kPut:
    case RequestParser::Method::kPatch:
      return true;
    default:
      return false;
  }
}

ResultOr<RequestParser::Method> ParseMethod(std::string_view method_str) {
  if (method_str == "GET") return RequestParser::Method::kGet;
  if (method_str == "POST") return RequestParser::Method::kPost;
  if (method_str == "HEAD") return RequestParser::Method::kHead;
  if (method_str == "DELETE") return RequestParser::Method::kDelete;
  if (method_str == "OPTIONS") return RequestParser::Method::kOptions;
  if (method_str == "TRACE") return RequestParser::Method::kTrace;
  if (method_str == "CONNECT") return RequestParser::Method::kConnect;
  if (method_str == "PUT") return RequestParser::Method::kPut;
  if (method_str == "PATCH") return RequestParser::Method::kPatch;
  return StreamFatalError(
      strings::Format("Unsupported or invalid HTTP method [{}]", method_str));
}

ResultOr<RequestParser::Version> ParseVersion(std::string_view version_str) {
  if (version_str == "HTTP/1.1") return RequestParser::Version::kHttp1point1;
  if (version_str == "HTTP/2.0" || version_str == "HTTP/2") {
    return RequestParser::Version::kHttp2point0;
  }
  return StreamFatalError(
      strings::Format("Unsupported or invalid HTTP version [{}]", version_str));
}

}  // namespace

ResultOr<size_t> RequestParser::Parse(std::string_view s) {
  // Find the end of headers (\r\n\r\n)
  const size_t header_end_pos = s.find("\r\n\r\n");
  if (header_end_pos == std::string_view::npos) {
    return size_t{0};
  }

  const std::string_view headers_part = s.substr(0, header_end_pos);

  // Parse request line
  const size_t first_line_end = headers_part.find("\r\n");
  std::string_view request_line;
  std::string_view remaining_headers;

  if (first_line_end == std::string_view::npos) {
    request_line = headers_part;
    remaining_headers = "";
  } else {
    request_line = headers_part.substr(0, first_line_end);
    remaining_headers = headers_part.substr(first_line_end + 2);
  }

  // Split request line by spaces: Method URI Version
  size_t first_space = request_line.find(' ');
  if (first_space == std::string_view::npos) {
    return StreamFatalError("Malformed HTTP request line: missing method");
  }
  std::string_view method_token = request_line.substr(0, first_space);

  size_t second_space = request_line.find(' ', first_space + 1);
  if (second_space == std::string_view::npos) {
    return StreamFatalError("Malformed HTTP request line: missing version");
  }
  std::string_view uri_token =
      request_line.substr(first_space + 1, second_space - (first_space + 1));
  std::string_view version_token = request_line.substr(second_space + 1);

  TRY_ASSIGN(Method parsed_method, ParseMethod(method_token));
  TRY_ASSIGN(Version parsed_version, ParseVersion(version_token));

  method_ = parsed_method;
  version_ = parsed_version;
  uri_ = std::string(uri_token);
  keyvals_.clear();

  // Parse headers
  size_t line_start = 0;
  while (line_start < remaining_headers.size()) {
    size_t line_end = remaining_headers.find("\r\n", line_start);
    if (line_end == std::string_view::npos) {
      line_end = remaining_headers.size();
    }

    std::string_view header_line =
        remaining_headers.substr(line_start, line_end - line_start);
    if (!header_line.empty()) {
      size_t colon_pos = header_line.find(':');
      if (colon_pos == std::string_view::npos) {
        return StreamFatalError(strings::Format(
            "Malformed header line: missing colon [{}]", header_line));
      }
      std::string_view key = header_line.substr(0, colon_pos);
      std::string_view val = header_line.substr(colon_pos + 1);

      // Trim leading/trailing whitespace from value
      while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) {
        val.remove_prefix(1);
      }
      while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) {
        val.remove_suffix(1);
      }

      // Peel off the common & expected headers.
      auto k = std::string(key);
      if (EqualsIgnoreCase(k, "Host")) {
        host_ = std::string(val);
      } else if (EqualsIgnoreCase(k, "User-Agent")) {
        user_agent_ = std::string(val);
      } else if (EqualsIgnoreCase(k, "Accept")) {
        accept_ = std::string(val);
      } else {
        keyvals_[k] = std::string(val);
      }
    }

    line_start = line_end + 2;
  }

  // Determine content length for body parsing
  size_t content_length = 0;
  for (const auto& [k, v] : keyvals_) {
    if (EqualsIgnoreCase(k, "Content-Length")) {
      ResultOr<int64_t> parsed_len = strings::ParseAs<int64_t>(v, 10);
      if (!parsed_len.IsOk() || parsed_len.ValueOrDie() < 0) {
        return StreamFatalError(
            strings::Format("Invalid Content-Length header value [{}]", v));
      }
      content_length = static_cast<size_t>(parsed_len.ValueOrDie());
      break;
    }
  }

  const size_t total_headers_size = header_end_pos + 4;  // include \r\n\r\n
  const size_t total_required_size = total_headers_size + content_length;

  if (!HasBody(method_)) {
    if (content_length > 0) {
      return StreamFatalError(strings::Format(
          "Invalid Content-Length on non-bodied method [{}]", content_length));
    }
  }

  if (s.size() < total_required_size) {
    // Body is not fully received yet; request more data (consume 0 bytes for
    // now)
    return size_t{0};
  }

  if (content_length > 0) {
    body_ = std::string(s.substr(total_headers_size, content_length));
  } else {
    body_.clear();
  }
  return total_required_size;
}

std::string RequestParser::ToString() const {
  std::string headers_str;
  for (const auto& [k, v] : keyvals()) {
    headers_str += strings::Format("{}: {}\r\n", k, v);
  }
  return strings::Format("{} {} {}\r\n{}\r\n{}", ::http::ToString(method()),
                         uri(), ::http::ToString(version()), headers_str,
                         body());
}

std::string ToString(RequestParser::Method method) {
  switch (method) {
    case RequestParser::Method::kGet:
      return "GET";
    case RequestParser::Method::kPost:
      return "POST";
    case RequestParser::Method::kHead:
      return "HEAD";
    case RequestParser::Method::kDelete:
      return "DELETE";
    case RequestParser::Method::kOptions:
      return "OPTIONS";
    case RequestParser::Method::kTrace:
      return "TRACE";
    case RequestParser::Method::kConnect:
      return "CONNECT";
    case RequestParser::Method::kPut:
      return "PUT";
    case RequestParser::Method::kPatch:
      return "PATCH";
  }
  return "UNKNOWN";
}

std::string ToString(RequestParser::Version version) {
  switch (version) {
    case RequestParser::Version::kHttp1point1:
      return "HTTP/1.1";
    case RequestParser::Version::kHttp2point0:
      return "HTTP/2.0";
  }
  return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, RequestParser::Method method) {
  return os << ToString(method);
}

std::ostream& operator<<(std::ostream& os, RequestParser::Version version) {
  return os << ToString(version);
}

}  // namespace http
