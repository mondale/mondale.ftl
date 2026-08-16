#ifndef BASE_ASYNC_SAFE_H_
#define BASE_ASYNC_SAFE_H_

#include <stdint.h>
#include <unistd.h>

namespace base::async_safe {

size_t StrLen(const char* s);
void MemCopy(void* dst, const void* src, size_t n);
bool StrEqualN(const char* a, const char* b, size_t n);
bool WriteAll(int fd, const char* buf, size_t len);
int EnumerateThreads(int* tids, int max_tids);

// Parses lowercase hex, advancing *p. Returns false if no digits present.
bool ParseHex(const char** p, const char* end, uint64_t* out);

class Writer final {
 public:
  explicit Writer(int fd) : fd_(fd) {}
  ~Writer() { Flush(); }

  void Bytes(const char* p, size_t n);
  void Str(const char* s) { Bytes(s, StrLen(s)); }
  void Char(char c) { Bytes(&c, 1); }
  void Hex64(uint64_t v);
  void Dec(long v);

  void Flush();

 private:
  int fd_;
  size_t len_ = 0;
};

}  // namespace base::async_safe

#endif  // #ifndef BASE_ASYNC_SAFE_H_
