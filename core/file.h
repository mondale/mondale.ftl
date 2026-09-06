#ifndef CORE_FILE_H_
#define CORE_FILE_H_

#include <string>
#include <string_view>

#include "core/result.h"

namespace core {

Result WriteContentsToFile(std::string_view file_name,
                           std::string_view contents);

ResultOr<std::string> ReadContentsFromFile(std::string_view file_name);

bool FileExists(std::string_view file_name);

bool FileIsReadable(std::string_view file_name);

bool FileIsWriteable(std::string_view file_name);

}  // namespace core

#endif  // #ifndef CORE_FILE_H_
