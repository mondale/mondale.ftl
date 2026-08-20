#ifndef CORE_IDIOMS_H_
#define CORE_IDIOMS_H_

#include <string_view>

#include "core/file_descriptor.h"
#include "core/result.h"

namespace core::idioms {

Result ReadExactly(const FileDescriptor& fd, char* output, size_t bytes);
Result WriteExactly(const FileDescriptor& fd, std::string_view data);

}  // namespace core::idioms

#endif  // #ifndef CORE_IDIOMS_H_
