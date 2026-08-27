#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "testing/scoped_temp_file.h"
#include "testing/testing.h"

TEST(ScopedTempFileCreation) {
  testing::ScopedTempFile temp_file;
  EXPECT_GE(temp_file.fd(), 0);
  EXPECT_FALSE(temp_file.filename().empty());

  struct stat st;
  EXPECT_EQ(stat(temp_file.filename().c_str(), &st), 0);
}

TEST(ScopedTempFileWriteAndRead) {
  testing::ScopedTempFile temp_file;
  const std::string_view kData = "hello c++26";
  ssize_t written = write(temp_file.fd(), kData.data(), 11);
  EXPECT_EQ(written, 11);

  int fd = open(temp_file.filename().c_str(), O_RDONLY);
  EXPECT_GE(fd, 0);
  char buf[32] = {0};
  ssize_t read_bytes = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  EXPECT_EQ(read_bytes, 11);
  EXPECT_EQ(buf, kData);
}

TEST(ScopedTempFileDestruction) {
  std::string filename;
  {
    testing::ScopedTempFile temp_file;
    filename = temp_file.filename();
    struct stat st;
    EXPECT_EQ(stat(filename.c_str(), &st), 0);
  }

  struct stat st;
  EXPECT_NE(stat(filename.c_str(), &st), 0);
}
