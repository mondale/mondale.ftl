#ifndef SCOPED_TEMP_FILE_H_
#define SCOPED_TEMP_FILE_H_

#include <string>
#include <string_view>

namespace testing {

// A ScopedTempFile picks a name and opens itself. It can then be used in one of
// two modes:
// - Use the fd as you like, or,
// - Call SetContentsAndClose(), leaving a file in the file system.
// Either way, the dtor cleans up and unlinks.
class ScopedTempFile final {
 public:
  ScopedTempFile();
  explicit ScopedTempFile(std::string_view contents);
  ~ScopedTempFile();

  ScopedTempFile(const ScopedTempFile&) = delete;
  ScopedTempFile& operator=(const ScopedTempFile&) = delete;
  ScopedTempFile(ScopedTempFile&&) = delete;
  ScopedTempFile& operator=(ScopedTempFile&&) = delete;

  // Do not invoke fd() after SetContentsAndClose().
  void SetContentsAndClose(std::string_view contents);

  int fd() const { return fd_; }
  const std::string& filename() const { return filename_; }

 private:
  int fd_ = -1;
  std::string filename_;
};

}  // namespace testing

#endif  // #ifndef SCOPED_TEMP_FILE_H_
