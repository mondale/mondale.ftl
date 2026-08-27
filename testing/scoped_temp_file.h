#ifndef SCOPED_TEMP_FILE_H_
#define SCOPED_TEMP_FILE_H_

#include <string>

namespace testing {

class ScopedTempFile final {
 public:
  ScopedTempFile();
  ~ScopedTempFile();

  ScopedTempFile(const ScopedTempFile&) = delete;
  ScopedTempFile& operator=(const ScopedTempFile&) = delete;
  ScopedTempFile(ScopedTempFile&&) = delete;
  ScopedTempFile& operator=(ScopedTempFile&&) = delete;

  int fd() const { return fd_; }
  const std::string& filename() const { return filename_; }

 private:
  int fd_ = -1;
  std::string filename_;
};

}  // namespace testing

#endif  // #ifndef SCOPED_TEMP_FILE_H_
