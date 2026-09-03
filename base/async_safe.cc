// Exempt from style expectations.

#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include "base/async_safe.h"
#include "base/raw_syscalls.h"

using namespace base::raw_syscalls;

namespace base::async_safe {

size_t StrLen(const char* s) {
  size_t n = 0;
  while (s[n] != '\0') ++n;
  return n;
}

void MemCopy(void* dst, const void* src, size_t n) {
  auto* d = static_cast<uint8_t*>(dst);
  const auto* s = static_cast<const uint8_t*>(src);
  for (size_t i = 0; i < n; ++i) d[i] = s[i];
}

bool StrEqualN(const char* a, const char* b, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Writes all of `buf`, retrying short writes and EINTR.  Returns false if the
// descriptor is unusable, at which point the dump is pointless anyway.
bool WriteAll(int fd, const char* buf, size_t len) {
  size_t done = 0;
  while (done < len) {
    long n = SysWrite(fd, buf + done, len - done);
    if (n > 0) {
      done += static_cast<size_t>(n);
      continue;
    }
    if (n == -EINTR) continue;
    return false;
  }
  return true;
}

// Buffered ASCII output.  The buffer is static (see the note on the state
// block below): a crash handler may be running on an alternate signal stack
// as small as MINSIGSTKSZ, so this file keeps its stack footprint tiny.
namespace {
char g_write_buf[1024];
}  // namespace

void Writer::Bytes(const char* p, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (len_ == sizeof(g_write_buf)) Flush();
    g_write_buf[len_++] = p[i];
  }
}
void Writer::Hex64(uint64_t v) {
  char tmp[18] = {'0', 'x'};
  for (int i = 0; i < 16; ++i) {
    tmp[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 0xf];
  }
  Bytes(tmp, 18);
}

void Writer::Dec(long v) {
  char tmp[24];
  int i = static_cast<int>(sizeof(tmp));
  bool neg = v < 0;
  unsigned long u =
      neg ? 0UL - static_cast<unsigned long>(v) : static_cast<unsigned long>(v);
  if (u == 0) tmp[--i] = '0';
  while (u != 0) {
    tmp[--i] = static_cast<char>('0' + (u % 10));
    u /= 10;
  }
  if (neg) tmp[--i] = '-';
  Bytes(tmp + i, sizeof(tmp) - static_cast<size_t>(i));
}

void Writer::Flush() {
  if (len_ != 0) {
    WriteAll(fd_, g_write_buf, len_);
    len_ = 0;
  }
}

bool ParseHex(const char** p, const char* end, uint64_t* out) {
  uint64_t v = 0;
  const char* q = *p;
  const char* start = q;
  while (q < end) {
    char c = *q;
    unsigned d;
    if (c >= '0' && c <= '9') {
      d = static_cast<unsigned>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      d = static_cast<unsigned>(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      d = static_cast<unsigned>(c - 'A' + 10);
    } else {
      break;
    }
    v = (v << 4) | d;
    ++q;
  }
  if (q == start) return false;
  *p = q;
  *out = v;
  return true;
}

int ParseFileContentsAsDecimal(const char* filename) {
  const int fd = SysOpen(filename, O_RDONLY);
  if (fd < 0) {
    return -1;
  }

  // Use a fixed stack buffer to avoid dynamic memory allocation.
  char buf[256];
  for (int i = 0; i < 256; ++i) buf[i] = 0;
  const ssize_t bytes_read = SysRead(fd, buf, sizeof(buf) - 1);

  // Always close the file descriptor promptly.
  SysClose(fd);

  if (bytes_read <= 0) {
    return -1;
  }

  buf[bytes_read] = '\0';

  // Manual string parsing to guarantee async-signal safety,
  // avoiding non-signal-safe libc routines like strtol, sscanf, or atoi.
  const char* p = buf;

  // Trim leading whitespace
  while (*p == ' ' || *p == '\t' || *p == '\r') {
    p++;
  }

  // Handle sign
  int sign = 1;
  if (*p == '-') {
    sign = -1;
    p++;
  } else if (*p == '+') {
    p++;
  }

  // Must contain at least one valid digit
  if (*p < '0' || *p > '9') {
    return -1;
  }

  long acc = 0;
  while (*p >= '0' && *p <= '9') {
    int digit = *p - '0';

    // Overflow check before multiplying
    if (acc > (INT_MAX - digit) / 10) {
      return -1;
    }

    acc = acc * 10 + digit;
    p++;
  }

  // Trim trailing whitespace and single-line newlines
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
    p++;
  }

  // Reject input if unparsed non-whitespace characters remain
  if (*p != '\0') {
    return -1;
  }

  long val = acc * sign;
  if (val < INT_MIN || val > INT_MAX) {
    return -1;
  }

  return static_cast<int>(val);
}

int EnumerateThreads(int* tids, int max_tids) {
  char scratch[8192];
  long fd = SysOpen("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) return 0;
  int n = 0;
  for (;;) {
    long len = SysGetdents64(static_cast<int>(fd), scratch, sizeof(scratch));
    if (len == -EINTR) continue;
    if (len <= 0) break;
    long off = 0;
    while (off < len && n < max_tids) {
      // struct linux_dirent64: d_ino(8) d_off(8) d_reclen(2) d_type(1) name
      uint16_t reclen;
      async_safe::MemCopy(&reclen, scratch + off + 16, sizeof(reclen));
      const char* name = scratch + off + 19;
      int tid = 0;
      bool ok = name[0] >= '0' && name[0] <= '9';
      for (const char* p = name; ok && *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
          ok = false;
          break;
        }
        tid = tid * 10 + (*p - '0');
      }
      if (ok) tids[n++] = tid;
      if (reclen == 0) break;
      off += reclen;
    }
    if (n >= max_tids) break;
  }
  SysClose(static_cast<int>(fd));
  return n;
}

}  // namespace base::async_safe
