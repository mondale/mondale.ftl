#ifndef HTTP_HTTP_STREAM_H_
#define HTTP_HTTP_STREAM_H_

#include "core/vocabulary.h"
#include "http/request_parser.h"
#include "io/io.h"

namespace http {

class HttpStream final {
 public:
  using Fn =
      std::move_only_function<void(Result, const RequestParser&, io::Poster*)>;
  HttpStream(io::Poster* p, Fn fn) : p_(p), fn_(std::move(fn)) {}

  void Prime();

 private:
  void AddBytes(size_t bytes);
  Result TryParse();
  void ReportSadIfNeeded();
  Result ShiftVector(size_t n);
  Result GrowVectorIfNeeded();
  void TrimVectorIfNeeded();

  Result r_ = Result::Ok();
  io::Poster* const p_;
  Fn fn_;
  std::vector<char> buf_;
  size_t bytes_ = 0;
};

}  // namespace http

#endif  // #ifndef HTTP_HTTP_STREAM_H_
