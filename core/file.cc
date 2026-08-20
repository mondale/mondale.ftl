#include "core/file.h"

namespace core {

Result WriteContentsToFile(std::string_view file_name,
                           std::string_view contents) {
  return Result::Ok();
}

ResultOr<std::string> ReadContentsFromFile(std::string_view file_name) {
  return "";
}

}  // namespace core
