#include <fcntl.h>
#include <unistd.h>

#include "core/file.h"
#include "core/vocabulary.h"
#include "testing/scoped_temp_file.h"

namespace testing {

ScopedTempFile::ScopedTempFile(std::string_view contents) : ScopedTempFile() {
  SetContentsAndClose(contents);
}

ScopedTempFile::ScopedTempFile() {
  char filename_template[] = "/tmp/scoped_temp_file_XXXXXX";
  fd_ = mkstemp(filename_template);
  if (fd_ >= 0) {
    filename_ = filename_template;
  }
}

ScopedTempFile::~ScopedTempFile() {
  if (fd_ >= 0) {
    close(fd_);
  }
  if (!filename_.empty()) {
    ::unlink(filename_.c_str());
  }
}

void ScopedTempFile::SetContentsAndClose(std::string_view contents) {
  CHECK_OK(core::WriteContentsToFile(filename_, contents));
}

}  // namespace testing
