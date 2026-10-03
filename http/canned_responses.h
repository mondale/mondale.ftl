#ifndef HTTP_CANNED_RESPONSES_H_
#define HTTP_CANNED_RESPONSES_H_

#include "core/vocabulary.h"
#include "http/request_parser.h"
#include "io/io.h"

namespace http {

class CannedResponses final {
 private:
  struct PrivateTag {};

 public:
  CannedResponses() = delete;
  explicit CannedResponses(PrivateTag);

  static CannedResponses* Instance();

  io::SourceBuffer NotFound404(const RequestParser& rp) const;

 private:
};

}  // namespace http

#endif  // #ifndef HTTP_CANNED_RESPONSES_H_
