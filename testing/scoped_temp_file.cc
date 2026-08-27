#include <fcntl.h>
#include <unistd.h>

#include "testing/scoped_temp_file.h"

namespace testing {

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
    ::unlink(filename_.c_str());
  }
}

}  // namespace testing
