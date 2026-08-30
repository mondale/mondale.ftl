#include <string.h>

#include "core/crc32c.h"
#include "testing/testing.h"

namespace {

TEST(Crc32c313233343536373839_0xE3069283) {
  EXPECT_EQ(core::CRC32C(0xE3069283), core::ComputeCRC32C("123456789", 9));
}

TEST(Crc32cEmptyString) {
  EXPECT_EQ(core::CRC32C(0x00000000), core::ComputeCRC32C("", 0));
}

TEST(Crc32cShort) {
  const char* const kDefinition = "CRC32C is a 32-bit cyclic redundancy check.";
  EXPECT_EQ(core::CRC32C(0xE1BD7F14),
            core::ComputeCRC32C(kDefinition, strlen(kDefinition)));
}

TEST(Crc32cDefinition) {
  const char* const kDefinition =
      "CRC32C is a 32-bit cyclic redundancy check using the Castagnoli "
      "polynomial (0x1EDC6F41). It offers superior error-detection performance "
      "compared to standard CRC32 and features dedicated hardware acceleration "
      "on modern Intel (SSE4.2) and ARM processors, making it fast and widely "
      "used in file systems, databases, and network protocols.";
  EXPECT_EQ(core::CRC32C(0xD33ED4E9),
            core::ComputeCRC32C(kDefinition, strlen(kDefinition)));
}

TEST(Crc32cZeros32Bytes) {
  // Block of 32 zero bytes
  char zeros[32] = {0};
  EXPECT_EQ(core::CRC32C(0x8A9136AA), core::ComputeCRC32C(zeros, 32));
}

TEST(Crc32cIncrementalUpdate) {
  core::CRC32C full_crc = core::ComputeCRC32C("123456789", 9);

  core::CRC32C part1 = core::ComputeCRC32C("1234", 4);
  core::CRC32C part2 = core::ComputeCRC32C("56789", 5, part1);

  EXPECT_EQ(full_crc, part2);
}

TEST(Crc32cDifferentWidthPaths) {
  const char* data = "abcdefghijk";  // 11 bytes
  core::CRC32C crc_all = core::ComputeCRC32C(data, 11);

  core::CRC32C crc_inc = core::ComputeCRC32C(data, 8);  // Exercises 8-byte path
  crc_inc = core::ComputeCRC32C(data + 8, 2, crc_inc);  // Exercises 2-byte path
  crc_inc =
      core::ComputeCRC32C(data + 10, 1, crc_inc);  // Exercises 1-byte path

  EXPECT_EQ(crc_all, crc_inc);
}

TEST(Crc32cFourBytePath) {
  const char* data = "abcd";  // 4 bytes
  core::CRC32C crc_direct = core::ComputeCRC32C(data, 4);

  core::CRC32C crc_step = core::ComputeCRC32C("ab", 2);
  crc_step = core::ComputeCRC32C("cd", 2, crc_step);

  EXPECT_EQ(crc_direct, crc_step);
}

}  // namespace
