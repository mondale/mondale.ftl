// Exempt from style expectations.
//
// Async-signal-safe whole-process stack dumper for Linux x86_64.
// See stack_dump.h for the contract.
//
// ---------------------------------------------------------------------------
// How this works
// ---------------------------------------------------------------------------
// Async-signal safety is obtained by not using libc at all: every kernel
// interaction below is a hand-rolled `syscall` instruction (Section 1).  That
// sidesteps the entire question of which glibc wrapper is or is not on the
// POSIX list, and it avoids errno, which is thread-local state a wrapper may
// touch.  The only libc facility used is <sys/syscall.h> and friends, which
// contribute macros and struct layouts, not code.
//
// Calling thread: unwound in process by walking the RBP chain, validated
// against /proc/self/maps at every step.  The first `kElidedSelfFrames`
// entries are dropped so the dumper's own frames do not appear.
//
// Other threads: the spec forbids interacting with them, which rules out both
// the signal-handshake approach (tgkill + handler cooperation) and the
// ptrace-from-a-_Fork()ed-child approach (PTRACE_ATTACH stops the tracee).
// So the registers are lifted non-invasively from
// /proc/self/task/<tid>/syscall, whose last two fields are the user-mode SP
// and PC of a thread that is blocked in the kernel.  (The kstkesp/kstkeip
// fields of /proc/*/stat, the other obvious source, have been hardcoded to 0
// since Linux 4.10 -- commit 0a1eb2d474ed.)  Given SP, the thread's stack is
// walked directly: all threads share our address space, so no cross-process
// memory access is needed.  A thread that is on-CPU reports "running" and has
// no recoverable registers; that is reported rather than guessed at.
//
// All memory reads of possibly-stale addresses go through process_vm_readv()
// on our own pid, which reports EFAULT for an unmapped page instead of
// raising SIGSEGV.  A stack that is unmapped concurrently by an exiting
// thread therefore truncates a trace; it cannot crash the dumper.
//
// Symbolisation: /usr/bin/addr2line is spawned once via _Fork()+execve() and
// driven as a coprocess over a pipe pair (binutils' addr2line fflush()es
// after each address specifically to support this).  Addresses are converted
// from run-time to link-time by subtracting the ELF load bias computed from
// /proc/self/maps plus the in-memory program headers, which is what makes the
// output correct under ASLR for both PIE and non-PIE binaries.

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <features.h>
#include <linux/limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include <atomic>

namespace {

// ---------------------------------------------------------------------------
// Tunables
// ---------------------------------------------------------------------------

constexpr int kMaxThreads = 1024;
constexpr int kMaxFrames = 64;
constexpr int kMaxRegions = 512;

// Bound on how much of another thread's stack is examined, so that a thread
// with a huge stack cannot make a crash handler run for an unbounded time.
constexpr uint64_t kMaxScanBytes = 512 * 1024;

// Frames elided from the calling thread's trace.  Walking outward from
// CaptureFrames(), the captured entries are:
//   [0] the frame executing DumpImpl()                       (dumper internal)
//   [1] the frame executing DumpAllStacksWithLineNumbers()   (dumper)
//   [2] the frame that called DumpAllStacksWithLineNumbers()
// The spec omits "the stacks associated with DumpAllStacksWithLineNumbers()
// and the stack that calls this method", i.e. all three -- hence 3.  If you
// read that sentence as keeping the immediate caller (which is what most
// backtrace APIs do), set this to 2; nothing else changes.
constexpr int kElidedSelfFrames = 3;

constexpr int kAddr2LineTimeoutMs = 3000;
constexpr char kAddr2LinePath[] = "/usr/bin/addr2line";

// ---------------------------------------------------------------------------
// Section 1: raw syscalls.  No libc, no errno; negative return == -errno.
// ---------------------------------------------------------------------------

inline long Sys(long nr, long a1 = 0, long a2 = 0, long a3 = 0, long a4 = 0,
                long a5 = 0, long a6 = 0) {
  long ret;
  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  register long r9 __asm__("r9") = a6;
  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "0"(nr), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
                     "r"(r9)
                   : "rcx", "r11", "memory");
  return ret;
}

long SysRead(int fd, void* buf, size_t n) {
  return Sys(__NR_read, fd, reinterpret_cast<long>(buf), static_cast<long>(n));
}
long SysWrite(int fd, const void* buf, size_t n) {
  return Sys(__NR_write, fd, reinterpret_cast<long>(buf), static_cast<long>(n));
}
long SysOpen(const char* path, int flags) {
  return Sys(__NR_openat, AT_FDCWD, reinterpret_cast<long>(path), flags, 0);
}
long SysClose(int fd) { return Sys(__NR_close, fd); }
long SysGetdents64(int fd, void* buf, size_t n) {
  return Sys(__NR_getdents64, fd, reinterpret_cast<long>(buf),
             static_cast<long>(n));
}
long SysReadlink(const char* path, char* buf, size_t n) {
  return Sys(__NR_readlinkat, AT_FDCWD, reinterpret_cast<long>(path),
             reinterpret_cast<long>(buf), static_cast<long>(n));
}
long SysPipe2(int fds[2], int flags) {
  return Sys(__NR_pipe2, reinterpret_cast<long>(fds), flags);
}
long SysDup3(int old_fd, int new_fd, int flags) {
  return Sys(__NR_dup3, old_fd, new_fd, flags);
}
long SysExecve(const char* path, char* const argv[], char* const envp[]) {
  return Sys(__NR_execve, reinterpret_cast<long>(path),
             reinterpret_cast<long>(argv), reinterpret_cast<long>(envp));
}
[[noreturn]] void SysExit(int code) {
  Sys(__NR_exit, code);
  __builtin_unreachable();
}
long SysWait4(int pid, int* status, int options) {
  return Sys(__NR_wait4, pid, reinterpret_cast<long>(status), options, 0);
}
long SysKill(int pid, int sig) { return Sys(__NR_kill, pid, sig); }
long SysGetpid() { return Sys(__NR_getpid); }
long SysGettid() { return Sys(__NR_gettid); }
long SysPoll(struct pollfd* fds, unsigned n, int timeout_ms) {
  return Sys(__NR_poll, reinterpret_cast<long>(fds), n, timeout_ms);
}
long SysProcessVmReadv(int pid, const struct iovec* local, unsigned long lcnt,
                       const struct iovec* remote, unsigned long rcnt) {
  return Sys(__NR_process_vm_readv, pid, reinterpret_cast<long>(local), lcnt,
             reinterpret_cast<long>(remote), rcnt, 0);
}
// The kernel's sigset_t is 8 bytes on x86_64; libc's is larger, so a raw
// uint64_t mask is passed with an explicit size of 8.
long SysSigprocmask(int how, const uint64_t* set, uint64_t* oldset) {
  return Sys(__NR_rt_sigprocmask, how, reinterpret_cast<long>(set),
             reinterpret_cast<long>(oldset), 8);
}
long SysSigtimedwait(const uint64_t* set, const struct timespec* ts) {
  return Sys(__NR_rt_sigtimedwait, reinterpret_cast<long>(set), 0,
             reinterpret_cast<long>(ts), 8);
}
void SleepMicros(long usec) {
  struct timespec ts = {usec / 1000000, (usec % 1000000) * 1000};
  Sys(__NR_nanosleep, reinterpret_cast<long>(&ts), 0);
}

// fork() is forbidden and would be unsafe anyway (pthread_atfork handlers run
// arbitrary user code).  _Fork() is the POSIX-2024 async-signal-safe variant;
// where glibc is too old, clone(SIGCHLD) is exactly what _Fork() issues.
long SafeFork() {
#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 34)
  return _Fork();
#else
  return Sys(__NR_clone, SIGCHLD, 0, 0, 0, 0);
#endif
#else
  return Sys(__NR_clone, SIGCHLD, 0, 0, 0, 0);
#endif
}

// ---------------------------------------------------------------------------
// Section 2: freestanding string / formatting helpers
// ---------------------------------------------------------------------------

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
char g_write_buf[1024];

class Writer {
 public:
  explicit Writer(int fd) : fd_(fd) {}
  ~Writer() { Flush(); }

  void Bytes(const char* p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
      if (len_ == sizeof(g_write_buf)) Flush();
      g_write_buf[len_++] = p[i];
    }
  }
  void Str(const char* s) { Bytes(s, StrLen(s)); }
  void Char(char c) { Bytes(&c, 1); }

  void Hex64(uint64_t v) {
    char tmp[18] = {'0', 'x'};
    for (int i = 0; i < 16; ++i) {
      tmp[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 0xf];
    }
    Bytes(tmp, 18);
  }

  void Dec(long v) {
    char tmp[24];
    int i = static_cast<int>(sizeof(tmp));
    bool neg = v < 0;
    unsigned long u = neg ? 0UL - static_cast<unsigned long>(v)
                          : static_cast<unsigned long>(v);
    if (u == 0) tmp[--i] = '0';
    while (u != 0) {
      tmp[--i] = static_cast<char>('0' + (u % 10));
      u /= 10;
    }
    if (neg) tmp[--i] = '-';
    Bytes(tmp + i, sizeof(tmp) - static_cast<size_t>(i));
  }

  void Flush() {
    if (len_ != 0) {
      WriteAll(fd_, g_write_buf, len_);
      len_ = 0;
    }
  }

 private:
  int fd_;
  size_t len_ = 0;
};

// Parses lowercase hex, advancing *p.  Returns false if no digits are present.
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

// ---------------------------------------------------------------------------
// Section 3: process-wide state, captured once per dump
// ---------------------------------------------------------------------------

struct Region {
  uint64_t lo, hi, file_off;
  bool readable, writable, executable;
  bool from_exe;  // Backed by the main executable's file.
};

// All dump state lives in .bss rather than on the stack, because a crash
// handler may be running on an alternate signal stack of only MINSIGSTKSZ
// bytes.  This is safe precisely because g_busy serialises the whole dump:
// exactly one call is in flight process-wide at any time, and a reentrant
// call (a signal arriving mid-dump on this same thread) is rejected rather
// than allowed to interleave.  Total footprint is ~50 KiB of .bss; the peak
// stack use of a dump is under 1 KiB.
Region g_regions[kMaxRegions];
int g_num_regions;
char g_exe_path[PATH_MAX];
size_t g_exe_path_len;
char g_exe_arg[64];  // "/proc/<pid>/exe"
int g_pid;
uint64_t g_load_bias;  // run-time addr - g_load_bias == link-time addr
bool g_exe_bias_valid;
bool g_have_pvr = true;  // process_vm_readv() available
volatile int g_busy;

char g_scratch[8192];             // /proc file reading, getdents64
char g_line_buf[PATH_MAX + 128];  // one line of /proc/self/maps
char g_sym_buf[PATH_MAX + 64];    // one addr2line reply
char g_a2l_buf[4096];             // addr2line read buffer
int g_tids[kMaxThreads];
uint64_t g_frames[kMaxFrames];

const Region* FindRegion(uint64_t addr) {
  // g_regions is sorted; /proc/*/maps is emitted in ascending order.
  int lo = 0, hi = g_num_regions - 1;
  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    if (addr < g_regions[mid].lo) {
      hi = mid - 1;
    } else if (addr >= g_regions[mid].hi) {
      lo = mid + 1;
    } else {
      return &g_regions[mid];
    }
  }
  return nullptr;
}

// Reads `len` bytes from our own address space without ever risking a fault:
// process_vm_readv() returns EFAULT for an unmapped page rather than
// delivering SIGSEGV.  A stack that is being torn down by an exiting thread
// truncates the trace instead of killing the process.
bool SafeRead(void* dst, uint64_t src, size_t len) {
  if (g_have_pvr) {
    struct iovec local = {dst, len};
    struct iovec remote = {reinterpret_cast<void*>(src), len};
    long n = SysProcessVmReadv(g_pid, &local, 1, &remote, 1);
    if (n == static_cast<long>(len)) return true;
    if (n != -ENOSYS && n != -EPERM) return false;
    g_have_pvr = false;  // Fall through to the map-validated path.
  }
  // Fallback: validate against the maps snapshot, then copy.  Racy in
  // principle (the mapping may vanish between the check and the copy) but the
  // only alternative without process_vm_readv is a SIGSEGV trampoline, which
  // would mean touching process-wide signal dispositions.
  const Region* r = FindRegion(src);
  if (r == nullptr || !r->readable || src + len > r->hi) return false;
  MemCopy(dst, reinterpret_cast<const void*>(src), len);
  return true;
}

bool SafeReadU64(uint64_t addr, uint64_t* out) {
  return SafeRead(out, addr, sizeof(*out));
}

// ---------------------------------------------------------------------------
// Section 4: /proc/self/maps
// ---------------------------------------------------------------------------

// "55a1c0f38000-55a1c0f39000 r-xp 00002000 08:01 1234  /path/to/exe"
void ParseMapsLine(const char* line, size_t len) {
  if (g_num_regions >= kMaxRegions) return;
  const char* p = line;
  const char* end = line + len;
  Region r = {};
  if (!ParseHex(&p, end, &r.lo)) return;
  if (p >= end || *p++ != '-') return;
  if (!ParseHex(&p, end, &r.hi)) return;
  if (p >= end || *p++ != ' ') return;
  if (end - p < 4) return;
  r.readable = p[0] == 'r';
  r.writable = p[1] == 'w';
  r.executable = p[2] == 'x';
  p += 4;
  if (p >= end || *p++ != ' ') return;
  if (!ParseHex(&p, end, &r.file_off)) return;

  // Skip dev and inode, then leading blanks, to reach the path.
  int fields = 0;
  while (p < end && fields < 2) {
    while (p < end && *p == ' ') ++p;
    while (p < end && *p != ' ') ++p;
    ++fields;
  }
  while (p < end && *p == ' ') ++p;

  size_t path_len = static_cast<size_t>(end - p);
  // A replaced/unlinked binary shows up as "<path> (deleted)"; still ours.
  const char kDeleted[] = " (deleted)";
  const size_t kDeletedLen = sizeof(kDeleted) - 1;
  if (path_len > kDeletedLen &&
      StrEqualN(p + path_len - kDeletedLen, kDeleted, kDeletedLen)) {
    path_len -= kDeletedLen;
  }
  r.from_exe = path_len == g_exe_path_len && g_exe_path_len != 0 &&
               StrEqualN(p, g_exe_path, path_len);
  g_regions[g_num_regions++] = r;
}

void LoadMaps() {
  g_num_regions = 0;
  long fd = SysOpen("/proc/self/maps", O_RDONLY | O_CLOEXEC);
  if (fd < 0) return;
  size_t line_len = 0;
  for (;;) {
    long n = SysRead(static_cast<int>(fd), g_scratch, sizeof(g_scratch));
    if (n == -EINTR) continue;
    if (n <= 0) break;
    for (long i = 0; i < n; ++i) {
      char c = g_scratch[i];
      if (c == '\n') {
        ParseMapsLine(g_line_buf, line_len);
        line_len = 0;
      } else if (line_len < sizeof(g_line_buf)) {
        g_line_buf[line_len++] = c;
      }
    }
  }
  if (line_len != 0) ParseMapsLine(g_line_buf, line_len);
  SysClose(static_cast<int>(fd));
}

// The load bias is what makes this ASLR-proof: addresses handed to addr2line
// must be link-time addresses.  For ET_EXEC the bias is zero; for ET_DYN
// (PIE) it is the mapped base minus the lowest PT_LOAD p_vaddr, which is
// almost always 0 but is computed properly for binaries linked with a
// non-zero text segment address.
void ComputeLoadBias() {
  g_exe_bias_valid = false;
  const Region* base = nullptr;
  for (int i = 0; i < g_num_regions; ++i) {
    if (g_regions[i].from_exe && g_regions[i].file_off == 0) {
      base = &g_regions[i];
      break;
    }
  }
  if (base == nullptr) return;

  Elf64_Ehdr eh;
  if (!SafeRead(&eh, base->lo, sizeof(eh))) return;
  if (eh.e_ident[EI_MAG0] != ELFMAG0 || eh.e_ident[EI_MAG1] != ELFMAG1 ||
      eh.e_ident[EI_MAG2] != ELFMAG2 || eh.e_ident[EI_MAG3] != ELFMAG3) {
    return;
  }
  if (eh.e_type == ET_EXEC) {
    g_load_bias = 0;
    g_exe_bias_valid = true;
    return;
  }
  if (eh.e_type != ET_DYN) return;

  uint64_t min_vaddr = ~0ULL;
  for (unsigned i = 0; i < eh.e_phnum && i < 128; ++i) {
    Elf64_Phdr ph;
    uint64_t off = base->lo + eh.e_phoff + i * eh.e_phentsize;
    if (!SafeRead(&ph, off, sizeof(ph))) return;
    if (ph.p_type == PT_LOAD && ph.p_vaddr < min_vaddr) min_vaddr = ph.p_vaddr;
  }
  if (min_vaddr == ~0ULL) return;
  g_load_bias = base->lo - min_vaddr;
  g_exe_bias_valid = true;
}

bool IsExecutableAddr(uint64_t pc) {
  const Region* r = FindRegion(pc);
  return r != nullptr && r->executable;
}

bool IsSymbolizableAddr(uint64_t pc) {
  const Region* r = FindRegion(pc);
  return r != nullptr && r->executable && r->from_exe && g_exe_bias_valid;
}

// ---------------------------------------------------------------------------
// Section 5: unwinding
// ---------------------------------------------------------------------------

// True if the eight bytes ending at `ra` can be the tail of a call
// instruction, i.e. if `ra` is plausibly a return address rather than an
// arbitrary code pointer that happens to be on the stack.  Recognises
// E8 rel32 and the FF /2, FF /3 register/memory-indirect forms with an
// optional REX prefix.  Heuristic by construction: conservative stack
// scanning has no ground truth to appeal to.
bool LooksLikeReturnAddress(uint64_t ra) {
  if (ra < 8 || !IsExecutableAddr(ra - 1)) return false;
  uint8_t b[8];
  if (!SafeRead(b, ra - 8, sizeof(b))) return false;
  // b[8 - k] is the byte at ra - k.
  if (b[3] == 0xE8) return true;  // call rel32 (5 bytes)
  for (int len = 2; len <= 7; ++len) {
    uint8_t op = b[8 - len];
    uint8_t modrm = b[8 - len + 1];
    int reg = (modrm >> 3) & 7;
    if (op == 0xFF && (reg == 2 || reg == 3)) return true;
    if (op >= 0x40 && op <= 0x4F && len >= 3) {  // REX prefix
      uint8_t op2 = b[8 - len + 1];
      uint8_t modrm2 = b[8 - len + 2];
      if (op2 == 0xFF && (((modrm2 >> 3) & 7) == 2 || ((modrm2 >> 3) & 7) == 3))
        return true;
    }
  }
  return false;
}

struct StackWindow {
  uint64_t lo, hi;
};

bool StackWindowFor(uint64_t sp, StackWindow* w) {
  const Region* r = FindRegion(sp);
  if (r == nullptr || !r->readable || !r->writable) return false;
  w->lo = r->lo;
  w->hi = r->hi;
  return true;
}

// Walks the saved-RBP chain.  `fp` points at a frame's saved RBP slot; the
// return address sits at fp + 8.  Crossing into a different mapping is
// allowed once (a handler running on an alternate signal stack chains back to
// the thread's real stack), but within a mapping the chain must ascend.
int WalkFramePointers(uint64_t fp, uint64_t* out, int max_frames) {
  int n = 0;
  uint64_t prev_fp = 0;
  StackWindow win = {0, 0};
  bool have_win = StackWindowFor(fp, &win);
  while (n < max_frames && fp != 0 && (fp & 7) == 0) {
    if (have_win && (fp < win.lo || fp + 16 > win.hi)) {
      if (!StackWindowFor(fp, &win)) break;  // Left every known stack.
      prev_fp = 0;                           // Restart monotonicity check.
    }
    if (prev_fp != 0 && fp <= prev_fp) break;
    uint64_t next_fp, ra;
    if (!SafeReadU64(fp, &next_fp)) break;
    if (!SafeReadU64(fp + 8, &ra)) break;
    if (ra == 0 || !IsExecutableAddr(ra)) break;
    out[n++] = ra;
    prev_fp = fp;
    fp = next_fp;
  }
  return n;
}

// Capture point for the calling thread.  noinline so the frame count above
// this function is exactly the constant kElidedSelfFrames accounts for.
__attribute__((noinline)) int CaptureFrames(uint64_t* out, int max_frames) {
  uint64_t fp = reinterpret_cast<uint64_t>(__builtin_frame_address(0));
  __asm__ volatile("" ::: "memory");
  return WalkFramePointers(fp, out, max_frames);
}

// Conservative scan of a stopped thread's stack: every aligned word from SP up
// to the top of the stack mapping that points just past a call instruction is
// treated as a return address.  False positives are possible (an uninitialised
// slot in a live frame can retain a return address written by an earlier,
// deeper call tree); false negatives are not, for frames whose return address
// is still on the stack.
int ScanStackConservatively(uint64_t sp, uint64_t* out, int max_frames) {
  StackWindow win;
  if (!StackWindowFor(sp, &win)) return 0;
  uint64_t addr = (sp + 7) & ~7ULL;
  uint64_t limit = win.hi;
  if (limit - addr > kMaxScanBytes) limit = addr + kMaxScanBytes;

  int n = 0;
  uint64_t prev_word = 0;
  while (addr + 8 <= limit && n < max_frames) {
    uint64_t word;
    if (!SafeReadU64(addr, &word)) break;
    // Collapse a value duplicated in *adjacent* slots (a return address that
    // was also spilled), but never collapse the same address appearing at
    // separated slots: that is ordinary recursion and each occurrence is a
    // real frame.
    if (word != prev_word && LooksLikeReturnAddress(word)) out[n++] = word;
    prev_word = word;
    addr += 8;
  }
  return n;
}

// Preferred strategy for a non-calling thread: find the innermost return
// address by scanning, then assume the standard prologue (push rbp; mov
// rbp,rsp) put the callee's saved RBP in the slot immediately below it and
// follow the chain from there.  Chains are far cleaner than scans, so this is
// tried first and accepted only if it produces a non-trivial trace.
int UnwindOtherThread(uint64_t sp, uint64_t pc, uint64_t* out, int max_frames,
                      bool* first_is_pc) {
  int n = 0;
  *first_is_pc = false;
  if (pc != 0 && IsExecutableAddr(pc)) {
    out[n++] = pc;
    *first_is_pc = true;
  }

  StackWindow win;
  if (!StackWindowFor(sp, &win)) return n;

  uint64_t addr = (sp + 7) & ~7ULL;
  uint64_t probe_limit = addr + 4096;
  if (probe_limit > win.hi) probe_limit = win.hi;
  for (; addr + 8 <= probe_limit; addr += 8) {
    uint64_t word;
    if (!SafeReadU64(addr, &word)) break;
    if (!LooksLikeReturnAddress(word)) continue;
    if (addr < 8) break;
    int m = WalkFramePointers(addr - 8, out + n, max_frames - n);
    if (m >= 3) return n + m;  // Chain held: trust it.
    break;
  }
  return n + ScanStackConservatively(sp, out + n, max_frames - n);
}

// ---------------------------------------------------------------------------
// Section 6: thread enumeration and register recovery
// ---------------------------------------------------------------------------

int EnumerateThreads(int* tids, int max_tids) {
  long fd = SysOpen("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) return 0;
  int n = 0;
  for (;;) {
    long len =
        SysGetdents64(static_cast<int>(fd), g_scratch, sizeof(g_scratch));
    if (len == -EINTR) continue;
    if (len <= 0) break;
    long off = 0;
    while (off < len && n < max_tids) {
      // struct linux_dirent64: d_ino(8) d_off(8) d_reclen(2) d_type(1) name
      uint16_t reclen;
      MemCopy(&reclen, g_scratch + off + 16, sizeof(reclen));
      const char* name = g_scratch + off + 19;
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

enum class RegStatus {
  kOk,       // SP and PC recovered.
  kRunning,  // Thread is on a CPU; its user registers are not recoverable.
  kGone,     // Thread exited between enumeration and inspection.
};

// Reads /proc/self/task/<tid>/syscall.  Content is either "running\n" or a
// list whose final two fields are the user-mode SP and PC.  Readable for
// threads in our own thread group without any ptrace privilege, because
// __ptrace_may_access() short-circuits for same_thread_group().
RegStatus ReadThreadRegisters(int tid, uint64_t* sp, uint64_t* pc) {
  char path[64];
  int i = 0;
  const char kPrefix[] = "/proc/self/task/";
  MemCopy(path, kPrefix, sizeof(kPrefix) - 1);
  i = sizeof(kPrefix) - 1;
  char digits[12];
  int d = 0;
  int t = tid;
  if (t == 0) digits[d++] = '0';
  while (t > 0) {
    digits[d++] = static_cast<char>('0' + t % 10);
    t /= 10;
  }
  while (d > 0) path[i++] = digits[--d];
  const char kSuffix[] = "/syscall";
  MemCopy(path + i, kSuffix, sizeof(kSuffix));
  i += static_cast<int>(sizeof(kSuffix)) - 1;

  long fd = SysOpen(path, O_RDONLY | O_CLOEXEC);
  if (fd == -ENOENT) return RegStatus::kGone;
  if (fd < 0) return RegStatus::kRunning;
  char buf[256];
  long n;
  do {
    n = SysRead(static_cast<int>(fd), buf, sizeof(buf) - 1);
  } while (n == -EINTR);
  SysClose(static_cast<int>(fd));
  if (n == -ESRCH) return RegStatus::kGone;
  if (n <= 0) return RegStatus::kRunning;
  buf[n] = '\0';
  if (buf[0] == 'r') return RegStatus::kRunning;  // "running"

  // Take the last two whitespace-separated tokens; this covers both the
  // 9-field (in a syscall) and 3-field (nr < 0) layouts.
  int end = static_cast<int>(n);
  while (end > 0 && (buf[end - 1] == '\n' || buf[end - 1] == ' ')) --end;
  int split[2] = {-1, -1};
  for (int j = end - 1, found = 0; j >= 0 && found < 2; --j) {
    if (buf[j] == ' ') split[found++] = j;
  }
  if (split[0] < 0 || split[1] < 0) return RegStatus::kRunning;
  const char* p1 = buf + split[1] + 1;
  const char* p2 = buf + split[0] + 1;
  if (p1[0] == '0' && p1[1] == 'x') p1 += 2;
  if (p2[0] == '0' && p2[1] == 'x') p2 += 2;
  const char* e1 = buf + split[0];
  const char* e2 = buf + end;
  if (!ParseHex(&p1, e1, sp) || !ParseHex(&p2, e2, pc)) {
    return RegStatus::kRunning;
  }
  return RegStatus::kOk;
}

// ---------------------------------------------------------------------------
// Section 7: addr2line coprocess
// ---------------------------------------------------------------------------

class Addr2Line {
 public:
  // Blocks SIGPIPE for this thread only (rt_sigprocmask is per-thread), so a
  // dead child turns a write into EPIPE instead of killing the process.
  bool Start() {
    uint64_t block = 1ULL << (SIGPIPE - 1);
    SysSigprocmask(SIG_BLOCK, &block, &saved_mask_);
    mask_saved_ = true;

    int to_child[2], from_child[2];
    if (SysPipe2(to_child, O_CLOEXEC) < 0) return false;
    if (SysPipe2(from_child, O_CLOEXEC) < 0) {
      SysClose(to_child[0]);
      SysClose(to_child[1]);
      return false;
    }
    long pid = SafeFork();
    if (pid < 0) {
      SysClose(to_child[0]);
      SysClose(to_child[1]);
      SysClose(from_child[0]);
      SysClose(from_child[1]);
      return false;
    }
    if (pid == 0) {
      // Child: only async-signal-safe syscalls between here and execve.
      SysDup3(to_child[0], 0, 0);
      SysDup3(from_child[1], 1, 0);
      long devnull = SysOpen("/dev/null", O_WRONLY);
      if (devnull >= 0) SysDup3(static_cast<int>(devnull), 2, 0);
      char arg0[] = "addr2line";
      char opt_e[] = "-e";
      char* argv[] = {arg0, opt_e, g_exe_arg, nullptr};
      char env0[] = "LC_ALL=C";  // Deterministic output for parsing.
      char* envp[] = {env0, nullptr};
      SysExecve(kAddr2LinePath, argv, envp);
      SysExit(127);
    }
    SysClose(to_child[0]);
    SysClose(from_child[1]);
    pid_ = static_cast<int>(pid);
    in_fd_ = to_child[1];
    out_fd_ = from_child[0];
    ok_ = true;
    return true;
  }

  // Translates a link-time address.  On any failure the coprocess is marked
  // dead and every later query fails fast.
  bool Query(uint64_t link_addr, char* out, size_t out_size) {
    if (!ok_) return false;
    char req[20] = {'0', 'x'};
    int i = 2;
    for (int shift = 60; shift >= 0; shift -= 4) {
      req[i++] = "0123456789abcdef"[(link_addr >> shift) & 0xf];
    }
    req[i++] = '\n';
    if (!WriteAll(in_fd_, req, static_cast<size_t>(i))) {
      Poison();
      return false;
    }
    return ReadLine(out, out_size);
  }

  void Stop() {
    if (in_fd_ >= 0) SysClose(in_fd_);  // EOF: addr2line exits.
    if (out_fd_ >= 0) SysClose(out_fd_);
    in_fd_ = out_fd_ = -1;
    if (pid_ > 0) {
      int status = 0;
      bool reaped = false;
      for (int i = 0; i < 200 && !reaped; ++i) {  // ~200 ms
        long r = SysWait4(pid_, &status, WNOHANG);
        if (r == pid_ || r == -ECHILD) {
          reaped = true;  // -ECHILD: another thread's wait() got there first.
        } else {
          SleepMicros(1000);
        }
      }
      if (!reaped) {
        SysKill(pid_, SIGKILL);
        SysWait4(pid_, &status, 0);
      }
      pid_ = -1;
    }
    if (mask_saved_) {
      // Drain a SIGPIPE we provoked before unblocking, so the process does
      // not take it once the mask is restored.
      if (got_epipe_) {
        uint64_t set = 1ULL << (SIGPIPE - 1);
        struct timespec zero = {0, 0};
        SysSigtimedwait(&set, &zero);
      }
      SysSigprocmask(SIG_SETMASK, &saved_mask_, nullptr);
      mask_saved_ = false;
    }
  }

  bool ok() const { return ok_; }

 private:
  void Poison() {
    got_epipe_ = true;
    ok_ = false;
  }

  // One line of addr2line output, with a bounded wait so that a wedged
  // coprocess cannot hang a crash handler.
  bool ReadLine(char* out, size_t out_size) {
    size_t n = 0;
    for (;;) {
      while (pos_ < len_) {
        char c = g_a2l_buf[pos_++];
        if (c == '\n') {
          out[n] = '\0';
          return true;
        }
        if (n + 1 < out_size) out[n++] = c;
      }
      struct pollfd pfd = {out_fd_, POLLIN, 0};
      long pr = SysPoll(&pfd, 1, kAddr2LineTimeoutMs);
      if (pr == -EINTR) continue;
      if (pr <= 0) {
        Poison();
        return false;
      }
      long r = SysRead(out_fd_, g_a2l_buf, sizeof(g_a2l_buf));
      if (r == -EINTR) continue;
      if (r <= 0) {
        Poison();
        return false;
      }
      pos_ = 0;
      len_ = static_cast<size_t>(r);
    }
  }

  int pid_ = -1;
  int in_fd_ = -1;
  int out_fd_ = -1;
  bool ok_ = false;
  bool mask_saved_ = false;
  bool got_epipe_ = false;
  uint64_t saved_mask_ = 0;
  size_t pos_ = 0, len_ = 0;
};

// ---------------------------------------------------------------------------
// Section 8: rendering
// ---------------------------------------------------------------------------

bool IsUnknownResult(const char* s) {
  return s[0] == '\0' || (s[0] == '?' && s[1] == '?');
}

// `first_is_pc` distinguishes a frame 0 that is a genuine program counter
// (lifted from a signal context or from /proc/.../syscall) from one that is a
// return address like every other entry.  The distinction matters: a return
// address must be symbolised as the call site one byte behind it, or a call
// that is the last instruction of a statement resolves to the following line
// -- or, for a call to a noreturn function, to a different function entirely.
void EmitFrames(Writer* w, Addr2Line* a2l, const uint64_t* frames, int count,
                bool first_is_pc) {
  for (int i = 0; i < count; ++i) {
    uint64_t pc = frames[i];
    w->Char('#');
    w->Dec(i);
    w->Char(' ');
    w->Hex64(pc);
    w->Char(' ');

    uint64_t query_pc = (i == 0 && first_is_pc) ? pc : pc - 1;
    bool resolved = false;
    if (IsSymbolizableAddr(query_pc) && a2l->ok()) {
      if (a2l->Query(query_pc - g_load_bias, g_sym_buf, sizeof(g_sym_buf))) {
        resolved = !IsUnknownResult(g_sym_buf);
        // addr2line appends " (discriminator N)" when several code paths
        // share a line.  Drop it: the spec's format is file name and line.
        for (size_t k = 0; g_sym_buf[k] != '\0'; ++k) {
          if (g_sym_buf[k] == ' ' && g_sym_buf[k + 1] == '(') {
            g_sym_buf[k] = '\0';
            break;
          }
        }
      }
    }
    if (resolved) {
      w->Str(g_sym_buf);
    } else {
      w->Str("[unknown/stripped]");
    }
    w->Char('\n');
  }
}

void EmitThreadHeader(Writer* w, int tid, bool calling) {
  w->Str("--- Thread ");
  w->Dec(tid);
  w->Str(calling ? " (Calling Thread) ---\n" : " ---\n");
}

// ---------------------------------------------------------------------------
// Section 9: entry points
// ---------------------------------------------------------------------------

__attribute__((noinline)) void DumpImpl(int fd, const ucontext_t* uc) {
  if (__atomic_exchange_n(&g_busy, 1, __ATOMIC_ACQ_REL) != 0) {
    static const char kBusy[] =
        "[stack dumper is already active on another thread]\n";
    WriteAll(fd, kBusy, sizeof(kBusy) - 1);
    return;
  }

  g_pid = static_cast<int>(SysGetpid());
  g_have_pvr = true;

  // /proc/self/exe must be resolved before forking: after execve the child's
  // /proc/self/exe is addr2line.  The path is used to match map entries; the
  // /proc/<pid>/exe form is handed to addr2line so that a binary which has
  // been renamed or unlinked still symbolises.
  long len = SysReadlink("/proc/self/exe", g_exe_path, sizeof(g_exe_path) - 1);
  g_exe_path_len = len > 0 ? static_cast<size_t>(len) : 0;
  g_exe_path[g_exe_path_len] = '\0';
  {
    const char kA[] = "/proc/";
    const char kB[] = "/exe";
    int i = static_cast<int>(sizeof(kA)) - 1;
    MemCopy(g_exe_arg, kA, sizeof(kA) - 1);
    char digits[12];
    int d = 0, t = g_pid;
    if (t == 0) digits[d++] = '0';
    while (t > 0) {
      digits[d++] = static_cast<char>('0' + t % 10);
      t /= 10;
    }
    while (d > 0) g_exe_arg[i++] = digits[--d];
    MemCopy(g_exe_arg + i, kB, sizeof(kB));
  }

  LoadMaps();
  ComputeLoadBias();

  Addr2Line a2l;
  a2l.Start();

  Writer w(fd);
  int self_tid = static_cast<int>(SysGettid());

  // --- calling thread -----------------------------------------------------
  int n;
  int elide;
  bool first_is_pc = false;
  if (uc != nullptr) {
    // Seed from the interrupted context so the faulting frame is reported.
    uint64_t rip = static_cast<uint64_t>(uc->uc_mcontext.gregs[REG_RIP]);
    uint64_t rbp = static_cast<uint64_t>(uc->uc_mcontext.gregs[REG_RBP]);
    uint64_t rsp = static_cast<uint64_t>(uc->uc_mcontext.gregs[REG_RSP]);
    n = 0;
    if (IsExecutableAddr(rip)) {
      g_frames[n++] = rip;
      first_is_pc = true;
    }
    // GCC omits the frame pointer for leaf functions even under
    // -fno-omit-frame-pointer, and a fault can also land on a function's very
    // first instruction, before its prologue has run.  In both cases RBP
    // still belongs to the caller and the interrupted function's return
    // address is the word at RSP, so the RBP walk alone would silently drop
    // one frame -- the one directly containing the crash.  Recover it.
    uint64_t at_rsp = 0, chain_ra = 0;
    if (SafeReadU64(rsp, &at_rsp) && LooksLikeReturnAddress(at_rsp) &&
        !(SafeReadU64(rbp + 8, &chain_ra) && chain_ra == at_rsp)) {
      g_frames[n++] = at_rsp;
    }
    n += WalkFramePointers(rbp, g_frames + n, kMaxFrames - n);
    elide = 0;
  } else {
    // Every entry of an RBP walk is a return address, including the first.
    n = CaptureFrames(g_frames, kMaxFrames);
    elide = kElidedSelfFrames;
  }
  EmitThreadHeader(&w, self_tid, /*calling=*/true);
  if (n > elide) {
    EmitFrames(&w, &a2l, g_frames + elide, n - elide, first_is_pc);
  } else {
    w.Str("#0 ");
    w.Hex64(0);
    w.Str(" [no frames above the dumper]\n");
  }

  // --- every other thread -------------------------------------------------
  int num_tids = EnumerateThreads(g_tids, kMaxThreads);
  for (int i = 0; i < num_tids; ++i) {
    int tid = g_tids[i];
    if (tid == self_tid) continue;
    w.Char('\n');
    EmitThreadHeader(&w, tid, /*calling=*/false);
    uint64_t sp = 0, pc = 0;
    RegStatus status = ReadThreadRegisters(tid, &sp, &pc);
    if (status != RegStatus::kOk) {
      w.Str("#0 ");
      w.Hex64(0);
      w.Str(status == RegStatus::kGone ? " [thread exited during dump]\n"
                                       : " [running; registers unavailable]\n");
      continue;
    }
    bool pc_is_frame0 = false;
    int count = UnwindOtherThread(sp, pc, g_frames, kMaxFrames, &pc_is_frame0);
    if (count == 0) {
      w.Str("#0 ");
      w.Hex64(pc);
      w.Str(" [unknown/stripped]\n");
    } else {
      EmitFrames(&w, &a2l, g_frames, count, pc_is_frame0);
    }
  }

  w.Flush();
  a2l.Stop();
  __atomic_store_n(&g_busy, 0, __ATOMIC_RELEASE);
}

}  // namespace

void DumpAllStacksWithLineNumbers(int fd) {
  DumpImpl(fd, nullptr);
  // Defeat the tail call: kElidedSelfFrames assumes this frame exists.
  __asm__ volatile("" ::: "memory");
}

void DumpAllStacksWithLineNumbers(int fd, const void* ucontext) {
  DumpImpl(fd, static_cast<const ucontext_t*>(ucontext));
  __asm__ volatile("" ::: "memory");
}

// Built-in self test for the stack dumper.
//
// This file is a test harness, not part of the async-signal-safe core: it
// uses pthreads and stdio freely.  Only the code in stack_dump.cc is under
// the signal-safety constraint, and BIST() exercises it in exactly the
// situations that constraint exists for, including from inside a real signal
// handler.
//
// The interesting checks are the line-number assertions.  Each level of the
// test call chain records the source line of its own call site; the dump is
// then required to name that exact line.  That single assertion covers the
// whole pipeline at once: RBP unwinding, the frame-elision constant, the
// run-time-to-link-time bias arithmetic under ASLR, and addr2line framing.

namespace {

constexpr int kBlockedWorkers = 3;
constexpr int kSpinWorkers = 1;
constexpr int kNumWorkers = kBlockedWorkers + kSpinWorkers;

int g_line_l2_calls_l1;
int g_line_l3_calls_l2;
int g_line_worker_blocks;
int g_line_handler_raise;

std::atomic<bool> g_stop_spinning{false};
std::atomic<int> g_workers_ready{0};

int g_signal_fd = -1;

void Barrier() { __asm__ volatile("" ::: "memory"); }

// --------------------------------------------------------------------------
// Worker threads, parked several frames deep in a blocking syscall.
// --------------------------------------------------------------------------

__attribute__((noinline)) void WorkerLevelC(int read_fd) {
  char c;
  g_workers_ready.fetch_add(1);
  // Deliberately not read(3): under _FORTIFY_SOURCE that is a static inline
  // in bits/unistd.h, and addr2line (correctly) attributes the address to the
  // innermost inline frame, which would make this assertion test glibc's
  // headers rather than the unwinder.
  g_line_worker_blocks = __LINE__ + 1;
  long n = syscall(SYS_read, read_fd, &c, 1);  // Parks here until teardown.
  Barrier();
  (void)n;
}

__attribute__((noinline)) void WorkerLevelB(int read_fd) {
  WorkerLevelC(read_fd);
  Barrier();
}

__attribute__((noinline)) void WorkerLevelA(int read_fd) {
  WorkerLevelB(read_fd);
  Barrier();
}

void* BlockedWorker(void* arg) {
  WorkerLevelA(*static_cast<int*>(arg));
  return nullptr;
}

void* SpinningWorker(void*) {
  g_workers_ready.fetch_add(1);
  // Exercises the "thread is on a CPU, registers unrecoverable" path.
  while (!g_stop_spinning.load(std::memory_order_relaxed)) {
  }
  return nullptr;
}

// --------------------------------------------------------------------------
// Calling-thread test chain.  Per the spec, the frames for
// DumpAllStacksWithLineNumbers() and for its immediate caller (BistLevel1)
// are elided, so the first reported frame must be BistLevel2's call site.
// --------------------------------------------------------------------------

__attribute__((noinline)) void BistLevel1(int fd) {
  DumpAllStacksWithLineNumbers(fd);
  Barrier();
}

__attribute__((noinline)) void BistLevel2(int fd) {
  g_line_l2_calls_l1 = __LINE__ + 1;
  BistLevel1(fd);
  Barrier();
}

__attribute__((noinline)) void BistLevel3(int fd) {
  g_line_l3_calls_l2 = __LINE__ + 1;
  BistLevel2(fd);
  Barrier();
}

// --------------------------------------------------------------------------
// Signal-handler test: the real reason the core exists.
// --------------------------------------------------------------------------

void SignalHandler(int, siginfo_t*, void* uc) {
  DumpAllStacksWithLineNumbers(g_signal_fd, uc);
}

__attribute__((noinline)) void RaiseFromHere() {
  g_line_handler_raise = __LINE__ + 1;
  raise(SIGUSR1);
  Barrier();
}

// --------------------------------------------------------------------------
// Harness plumbing
// --------------------------------------------------------------------------

int MakeCaptureFd() {
  long fd = syscall(SYS_memfd_create, "stackdump-bist", 0);
  return static_cast<int>(fd);
}

// Reads a memfd back into `out`; returns bytes read.
size_t SlurpFd(int fd, char* out, size_t out_size) {
  lseek(fd, 0, SEEK_SET);
  size_t total = 0;
  for (;;) {
    ssize_t n = read(fd, out + total, out_size - 1 - total);
    if (n <= 0) break;
    total += static_cast<size_t>(n);
    if (total + 1 >= out_size) break;
  }
  out[total] = '\0';
  return total;
}

int g_checks_run = 0;
int g_checks_passed = 0;

void Check(bool ok, const char* what) {
  ++g_checks_run;
  if (ok) ++g_checks_passed;
  printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
}

// Counts occurrences of `needle` in `hay`.
int CountOf(const char* hay, const char* needle) {
  int n = 0;
  for (const char* p = strstr(hay, needle); p != nullptr;
       p = strstr(p + 1, needle)) {
    ++n;
  }
  return n;
}

// True if `line` ends in ":<expected>" (optionally with a discriminator).
bool LineNumberIs(const char* line, int expected) {
  const char* colon = strrchr(line, ':');
  if (colon == nullptr) return false;
  return atoi(colon + 1) == expected;
}

// Copies the n'th line of `text` into `out`.
bool GetLine(const char* text, int index, char* out, size_t out_size) {
  const char* p = text;
  for (int i = 0; i < index; ++i) {
    p = strchr(p, '\n');
    if (p == nullptr) return false;
    ++p;
  }
  const char* end = strchr(p, '\n');
  size_t len = end != nullptr ? static_cast<size_t>(end - p) : strlen(p);
  if (len >= out_size) len = out_size - 1;
  memcpy(out, p, len);
  out[len] = '\0';
  return true;
}

}  // namespace

void BIST() {
  printf("=== stack dumper BIST ===\n\n");

  static char dump[512 * 1024];
  static char sig_dump[512 * 1024];
  char line[4096];

  // ---- set up worker threads in known states -----------------------------
  int park_pipe[2];
  if (pipe(park_pipe) != 0) {
    printf("BIST: pipe() failed\n");
    return;
  }
  pthread_t workers[kNumWorkers];
  for (int i = 0; i < kBlockedWorkers; ++i) {
    pthread_create(&workers[i], nullptr, BlockedWorker, &park_pipe[0]);
  }
  for (int i = 0; i < kSpinWorkers; ++i) {
    pthread_create(&workers[kBlockedWorkers + i], nullptr, SpinningWorker,
                   nullptr);
  }
  while (g_workers_ready.load() < kNumWorkers) {
    struct timespec ts = {0, 1000000};
    nanosleep(&ts, nullptr);
  }
  // Let the blocked workers actually reach the read() syscall.
  struct timespec settle = {0, 50 * 1000 * 1000};
  nanosleep(&settle, nullptr);

  // ---- run the dumper from a nested call chain ---------------------------
  int fd = MakeCaptureFd();
  if (fd < 0) {
    printf("BIST: memfd_create failed\n");
    return;
  }
  BistLevel3(fd);
  SlurpFd(fd, dump, sizeof(dump));
  close(fd);

  // ---- run the dumper from inside a real signal handler ------------------
  struct sigaction sa = {};
  sa.sa_sigaction = SignalHandler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, nullptr);
  g_signal_fd = MakeCaptureFd();
  RaiseFromHere();
  SlurpFd(g_signal_fd, sig_dump, sizeof(sig_dump));
  close(g_signal_fd);

  // ---- tear down ---------------------------------------------------------
  g_stop_spinning.store(true);
  for (int i = 0; i < kBlockedWorkers; ++i) {
    char c = 'x';
    ssize_t w = write(park_pipe[1], &c, 1);
    (void)w;
  }
  for (int i = 0; i < kNumWorkers; ++i) pthread_join(workers[i], nullptr);
  close(park_pipe[0]);
  close(park_pipe[1]);

  // ---- report ------------------------------------------------------------
  printf("--- captured dump (normal call path) ---\n%s\n", dump);
  printf("--- captured dump (from SIGUSR1 handler) ---\n%s\n", sig_dump);

  printf("checks:\n");

  char header[256];
  snprintf(header, sizeof(header), "--- Thread %d (Calling Thread) ---",
           static_cast<int>(syscall(SYS_gettid)));
  Check(strncmp(dump, header, strlen(header)) == 0,
        "calling thread is dumped first and labelled");

  Check(CountOf(dump, "--- Thread ") == kNumWorkers + 1,
        "every thread in the process appears exactly once");

  Check(CountOf(dump, " (Calling Thread) ---") == 1,
        "exactly one thread is marked as the caller");

  bool got_f0 = GetLine(dump, 1, line, sizeof(line));
  Check(got_f0 && strncmp(line, "#0 0x", 5) == 0 &&
            strstr(line, "stack_dump_bist.cc") != nullptr &&
            LineNumberIs(line, g_line_l2_calls_l1),
        "frame #0 is BistLevel2's call site (elision + bias + addr2line)");
  if (!got_f0 || !LineNumberIs(line, g_line_l2_calls_l1)) {
    printf("        expected line %d, got: %s\n", g_line_l2_calls_l1, line);
  }

  bool got_f1 = GetLine(dump, 2, line, sizeof(line));
  Check(got_f1 && LineNumberIs(line, g_line_l3_calls_l2),
        "frame #1 is BistLevel3's call site (chain continues correctly)");
  if (!got_f1 || !LineNumberIs(line, g_line_l3_calls_l2)) {
    printf("        expected line %d, got: %s\n", g_line_l3_calls_l2, line);
  }

  Check(strstr(dump, "stack_dump.cc") == nullptr,
        "no frame of the dumper itself leaks into the output");

  // The worker threads park inside WorkerLevelC's read(); its return address
  // is on the stack, so the call site must be recovered cross-thread.
  char want[64];
  snprintf(want, sizeof(want), "stack_dump_bist.cc:%d", g_line_worker_blocks);
  Check(CountOf(dump, want) >= kBlockedWorkers,
        "blocked worker threads are unwound to their exact call site");

  Check(strstr(dump, "[running; registers unavailable]") != nullptr,
        "an on-CPU thread is reported honestly, not guessed at");

  Check(CountOf(sig_dump, "--- Thread ") == kNumWorkers + 1 &&
            strstr(sig_dump, "(Calling Thread)") != nullptr,
        "dumper works when called from inside a signal handler");

  snprintf(want, sizeof(want), "stack_dump_bist.cc:%d", g_line_handler_raise);
  Check(strstr(sig_dump, want) != nullptr,
        "ucontext seeding recovers the interrupted frame");

  Check(strstr(dump, "[unknown/stripped]") != nullptr,
        "non-executable-owned frames degrade to [unknown/stripped]");

  printf("\n%d/%d checks passed\n", g_checks_passed, g_checks_run);
}

int main(int argc, char* argv[]) {
  BIST();
  return 0;
}
