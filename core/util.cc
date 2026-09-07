#include "base/logging.h"
#include "base/process.h"
#include "core/util.h"

namespace core::util {

void DieElegantlyIfNotOk(Result r, base::SourceLocation loc) {
  if (IsOk(r)) return;
  std::stringstream s;
  s << "Termination at [" << loc.relative_file_name() << ":" << loc.line()
    << ": " << r;
  Log(ERROR) << s.str();
  std::cerr << s.str() << std::endl;
  base::FlushLogs();
  exit(1);
}

}  // namespace core::util
