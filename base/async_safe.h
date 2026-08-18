#ifndef BASE_ASYNC_SAFE_H_
#define BASE_ASYNC_SAFE_H_

#include <stdint.h>
#include <unistd.h>

namespace base::async_safe {

// Returns length of the NUL-terminated string (number of bytes before '\0').
size_t StrLen(const char* s);

// Copies `n` bytes from `src` to `dst` using a simple byte-wise copy.
void MemCopy(void* dst, const void* src, size_t n);

// Compares the first `n` bytes of `a` and `b` for equality.
bool StrEqualN(const char* a, const char* b, size_t n);

// Writes `len` bytes from `buf` to `fd`, retrying short writes and EINTR;
// returns true on success.
bool WriteAll(int fd, const char* buf, size_t len);

// Enumerates thread IDs by reading /proc/self/task, storing up to `max_tids`
// into `tids` and returning the count.
int EnumerateThreads(int* tids, int max_tids);

// Parses a hexadecimal number (digits [0-9a-fA-F]) from *p up to `end`,
// advancing *p on success. Returns false if no hex digits were present.
bool ParseHex(const char** p, const char* end, uint64_t* out);

// Opens 'filename' and parses the contents as a decimal integer. Returns -1 on
// error.
int ParseFileContentsAsDecimal(const char* filename);

// Buffered ASCII writer that accumulates output and flushes it to the provided
// file descriptor.
class Writer final {
 public:
  explicit Writer(int fd) : fd_(fd) {}
  ~Writer() { Flush(); }

  // Append `n` bytes from `p` into the internal buffer.
  void Bytes(const char* p, size_t n);
  // Append a NUL-terminated string.
  void Str(const char* s) { Bytes(s, StrLen(s)); }
  // Append a single character.
  void Char(char c) { Bytes(&c, 1); }
  // Append "0x"-prefixed 64-bit hexadecimal representation of `v`.
  void Hex64(uint64_t v);
  // Append decimal representation of `v`.
  void Dec(long v);

  // Flush any buffered bytes to the file descriptor immediately.
  void Flush();

 private:
  int fd_;
  size_t len_ = 0;
};

}  // namespace base::async_safe

#endif  // #ifndef BASE_ASYNC_SAFE_H_
