#ifndef HTTP_REQUEST_PARSER_H_
#define HTTP_REQUEST_PARSER_H_

#include <string>
#include <unordered_map>

#include "core/vocabulary.h"

namespace http {

class RequestParser final {
 public:
  enum class Method {
    kGet,
    kPost,
    kHead,
    kDelete,
    kOptions,
    kTrace,
    kConnect,
    kPut,
    kPatch,
  };

  enum class Version {
    kHttp1point1,
    kHttp2point0,
  };

  // Returns an error if 's' can never be parsed as HTTP.
  //
  // Returns 0 if 's' is not sufficiently terminated to currently parse as http,
  // signalling "consume zero bytes".
  //
  // Signals a successful parse by returning the number of bytes to trim from
  // the front of 's' to account for all header and body bytes.
  ResultOr<size_t> Parse(std::string_view s);

  std::string ToString() const;

  Method method() const { return method_; }
  Version version() const { return version_; }
  const std::string& uri() const { return uri_; }
  const std::string& body() const { return body_; }
  const auto& keyvals() const { return keyvals_; }

 private:
  Method method_ = Method::kGet;
  Version version_ = Version::kHttp1point1;
  std::string uri_;
  std::string body_;
  std::unordered_map<std::string, std::string> keyvals_;
};

std::string ToString(RequestParser::Method method);
std::string ToString(RequestParser::Version version);

std::ostream& operator<<(std::ostream& os, RequestParser::Method method);
std::ostream& operator<<(std::ostream& os, RequestParser::Version version);

}  // namespace http

#endif  // #ifndef HTTP_REQUEST_PARSER_H_
