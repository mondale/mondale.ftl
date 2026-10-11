#ifndef HTTP_HANDLERS_STATUSZ_H_
#define HTTP_HANDLERS_STATUSZ_H_

#include "core/vocabulary.h"
#include "http/request_parser.h"

namespace http::handlers {

ResultOr<std::string> ServeStatusz(const RequestParser& rp);

}  // namespace http::handlers

#endif  // #ifndef HTTP_HANDLERS_STATUSZ_H_
