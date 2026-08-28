#include <atomic>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "base/logging.h"

namespace base {
namespace internal {

struct VmoduleConfig {
  std::unordered_map<std::string, int> patterns;
};

// Global atomic pointer for lock-free reads.
std::atomic<const VmoduleConfig*> g_vmodule_config{nullptr};
std::atomic<int> g_logging_verbosity{0};

namespace {
// Zero-allocation helper to extract file stem from path string_view
std::string_view GetFileStem(std::string_view path) {
  if (path.empty()) return {};

  auto slash = path.find_last_of("/\\");
  std::string_view filename =
      (slash == std::string_view::npos) ? path : path.substr(slash + 1);

  auto dot = filename.find_last_of('.');
  return (dot == std::string_view::npos) ? filename : filename.substr(0, dot);
}
}  // namespace

bool VlogIsOnSlow(int level, base::SourceLocation loc) {
  const VmoduleConfig* config =
      g_vmodule_config.load(std::memory_order_acquire);
  const int enabled_verbosity =
      g_logging_verbosity.load(std::memory_order_relaxed);
  if (!config || config->patterns.empty()) {
    // When there is no vmodule, it's just a check on vlog-level.
    return enabled_verbosity >= level;
  }

  const std::string_view stem = GetFileStem(loc.file());

  // Check exact stem match
  auto it = config->patterns.find(std::string(stem));
  if (it != config->patterns.end()) {
    return it->second >= level;
  }
  return false;
}

}  // namespace internal

void SetVlogLevel(int level) {
  internal::g_logging_verbosity.store(level, std::memory_order_relaxed);
}

void SetVmodules(std::string_view vmodules) {
  if (internal::g_vmodule_config.load(std::memory_order_relaxed) != nullptr) {
    std::cerr
        << "Warning: VModule already set? Multiple calls to SetVmodules()?"
        << std::endl;
    return;
  }

  auto* config = new internal::VmoduleConfig();

  size_t start = 0;
  while (start < vmodules.size()) {
    size_t comma = vmodules.find(',', start);
    if (comma == std::string_view::npos) {
      comma = vmodules.size();
    }

    std::string_view entry = vmodules.substr(start, comma - start);
    size_t eq = entry.find('=');
    if (eq != std::string_view::npos) {
      std::string_view name = entry.substr(0, eq);
      std::string_view level_str = entry.substr(eq + 1);

      int level = 0;
      std::from_chars(level_str.data(), level_str.data() + level_str.size(),
                      level);
      config->patterns[std::string(name)] = level;
    }

    start = comma + 1;
  }

  internal::g_vmodule_config.store(config, std::memory_order_release);
}

}  // namespace base
