#include <format>
#include <string>
#include <string_view>

#include "base/flags.h"

namespace base::internal {

namespace {

struct FlagRegistryState {
  FlagRegistrationNode* head = nullptr;
};

FlagRegistryState* GetFlagRegistryState() {
  static FlagRegistryState* instance = new FlagRegistryState();
  return instance;
}

}  // namespace

void RegisterFlag(FlagRegistrationNode* node) {
  FlagRegistryState* registry = GetFlagRegistryState();
  node->next = registry->head;
  registry->head = node;
}

}  // namespace base::internal

namespace base {

bool ParseFlags(int argc, char* argv[], std::string* err) {
  auto* const registry = internal::GetFlagRegistryState();

  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (!arg.starts_with("--")) {
      continue;
    }
    arg.remove_prefix(2);

    auto eq_pos = arg.find('=');
    if (eq_pos == std::string_view::npos) {
      if (err) {
        *err = std::format("Invalid flag format (missing '='): --{}", arg);
      }
      return false;
    }

    std::string_view full_name = arg.substr(0, eq_pos);
    std::string_view val = arg.substr(eq_pos + 1);

    bool found = false;
    for (auto* node = registry->head; node != nullptr; node = node->next) {
      std::string qualified_name = std::string(node->cohort) + "." + node->name;
      if (qualified_name == full_name) {
        found = true;
        if (!node->parse_and_validate(val, err, node->builder_ptr)) {
          return false;
        }
        break;
      }
    }

    if (!found) {
      if (err) {
        *err = std::format("Unknown flag: --{}", full_name);
      }
      return false;
    }
  }
  return true;
}

}  // namespace base
