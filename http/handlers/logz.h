#ifndef HTTP_HANDLERS_LOGZ_H_
#define HTTP_HANDLERS_LOGZ_H_

#include <string>

#include "core/vocabulary.h"
#include "http/request_parser.h"

namespace http::handlers {

// Serves HTTP requests for /logz according to the logz specification.
// Parses log files, applies filtering/pagination rules, and produces
// either interactive dark-themed HTML (for browser clients) or raw plain-text
// logs (for non-browser CLI clients like curl/wget).
ResultOr<std::string> ServeLogz(const RequestParser& rp);

}  // namespace http::handlers

#endif  // HTTP_HANDLERS_LOGZ_H_
