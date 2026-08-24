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

#include "base/stacktrace.h"
#include "testing/testing.h"

using base::stacktrace::DumpAllStacksWithLineNumbers;

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

TEST(StacktraceBist) {
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

}  // namespace
