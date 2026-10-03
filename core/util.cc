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

Encumbered::~Encumbered() {
  std::list<std::move_only_function<void()>> e;
  {
    base::MutexLock l(&mu_);
    encumbered_.swap(e);
  }
  for (auto& fn : e) {
    fn();
  }
}

void Encumbered::Encumber(std::move_only_function<void()> fn) {
  // Add at front for LIFO.
  base::MutexLock l(&mu_);
  encumbered_.emplace_front(std::move(fn));
}

}  // namespace core::util
