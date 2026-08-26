#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include "base/logging_internal.h"

#define INFO (::base::internal::LogSeverity::kInfo)
#define WARNING (::base::internal::LogSeverity::kWarning)
#define ERROR (::base::internal::LogSeverity::kError)
#define FATAL (::base::internal::LogSeverity::kFatal)

#endif  // #ifndef BASE_LOGGING_H_
