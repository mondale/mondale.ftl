// Stacktrace dumper. Credit to Claude.
//
// Claude's comments, which are a little bit wrong after refactoring to several
// files:
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
//
// Mondale's follow-up: Before running the stackdump routine, we'll usually send
// SIGSTOP to all threads.

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

#include "base/async_safe.h"
#include "base/raw_syscalls.h"
#include "base/stacktrace.h"

namespace base::stacktrace {

using namespace base::raw_syscalls;

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
// Process-wide state, captured once per dump
// ---------------------------------------------------------------------------

struct Region {
  uint64_t lo;
  uint64_t hi;
  uint64_t file_off;
  bool readable;
  bool writable;
  bool executable;
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

char g_scratch[8192];             // /proc file reading
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
  async_safe::MemCopy(dst, reinterpret_cast<const void*>(src), len);
  return true;
}

bool SafeReadU64(uint64_t addr, uint64_t* out) {
  return SafeRead(out, addr, sizeof(*out));
}

// "55a1c0f38000-55a1c0f39000 r-xp 00002000 08:01 1234  /path/to/exe"
void ParseMapsLine(const char* line, size_t len) {
  if (g_num_regions >= kMaxRegions) return;
  const char* p = line;
  const char* end = line + len;
  Region r = {};
  if (!async_safe::ParseHex(&p, end, &r.lo)) return;
  if (p >= end || *p++ != '-') return;
  if (!async_safe::ParseHex(&p, end, &r.hi)) return;
  if (p >= end || *p++ != ' ') return;
  if (end - p < 4) return;
  r.readable = p[0] == 'r';
  r.writable = p[1] == 'w';
  r.executable = p[2] == 'x';
  p += 4;
  if (p >= end || *p++ != ' ') return;
  if (!async_safe::ParseHex(&p, end, &r.file_off)) return;

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
      async_safe::StrEqualN(p + path_len - kDeletedLen, kDeleted,
                            kDeletedLen)) {
    path_len -= kDeletedLen;
  }
  r.from_exe = path_len == g_exe_path_len && g_exe_path_len != 0 &&
               async_safe::StrEqualN(p, g_exe_path, path_len);
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
// unwinding
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
  uint64_t lo;
  uint64_t hi;
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
  async_safe::MemCopy(path, kPrefix, sizeof(kPrefix) - 1);
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
  async_safe::MemCopy(path + i, kSuffix, sizeof(kSuffix));
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
  if (!async_safe::ParseHex(&p1, e1, sp) ||
      !async_safe::ParseHex(&p2, e2, pc)) {
    return RegStatus::kRunning;
  }
  return RegStatus::kOk;
}

// ---------------------------------------------------------------------------
// addr2line coprocess
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
    if (!async_safe::WriteAll(in_fd_, req, static_cast<size_t>(i))) {
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
void EmitFrames(async_safe::Writer* w, Addr2Line* a2l, const uint64_t* frames,
                int count, bool first_is_pc) {
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

void EmitThreadHeader(async_safe::Writer* w, int tid, bool calling) {
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
    async_safe::WriteAll(fd, kBusy, sizeof(kBusy) - 1);
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
    async_safe::MemCopy(g_exe_arg, kA, sizeof(kA) - 1);
    char digits[12];
    int d = 0, t = g_pid;
    if (t == 0) digits[d++] = '0';
    while (t > 0) {
      digits[d++] = static_cast<char>('0' + t % 10);
      t /= 10;
    }
    while (d > 0) g_exe_arg[i++] = digits[--d];
    async_safe::MemCopy(g_exe_arg + i, kB, sizeof(kB));
  }

  LoadMaps();
  ComputeLoadBias();

  Addr2Line a2l;
  a2l.Start();

  async_safe::Writer w(fd);
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
  int num_tids = async_safe::EnumerateThreads(g_tids, kMaxThreads);
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

}  // namespace base::stacktrace
