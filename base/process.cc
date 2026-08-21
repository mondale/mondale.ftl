#include <execinfo.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <iostream>

#include "base/async_safe.h"
#include "base/process.h"
#include "base/raw_syscalls.h"
#include "base/stacktrace.h"

using namespace base::raw_syscalls;

namespace base {
namespace {

const int kCaptureSignal = SIGRTMIN + 1;
std::atomic<int> global_stopped_count{0};
std::atomic<int> global_futex_flag{0};

void CapturedThreadHandler(int sig) {
  global_stopped_count.fetch_add(1, std::memory_order_release);
  SysFutex(&global_futex_flag, FUTEX_WAIT_PRIVATE);
}

void AwaitCapturedThreads(int count) {
  int attempts = 10000;
  while (global_stopped_count.load(std::memory_order_acquire) < count &&
         attempts--) {
    SysSchedYield();
  }
}

const char* SigToName(int sig) {
  switch (sig) {
    case SIGSEGV:
      return "SIGSEGV";
    case SIGABRT:
      return "SIGABRT";
    case SIGBUS:
      return "SIGBUS";
    case SIGILL:
      return "SIGILL";
    case SIGFPE:
      return "SIGFPE";
    default:
      return "SIG_IDK";
  }
}

void SignalThreads(const int* tids, int thread_count, int sig) {
  for (int i = 0; i < thread_count; ++i) {
    SysTKill(tids[i], sig);
  }
}

int EnumerateOtherThreads(int* tids, int max_threads) {
  const int self = static_cast<int>(SysGettid());
  const int thread_count = async_safe::EnumerateThreads(tids, 1024);
  for (int i = 0; i < thread_count; ++i) {
    if (self == tids[i]) {
      tids[i] = tids[thread_count - 1];
      return thread_count - 1;
    }
  }
  return thread_count;
}

[[noreturn]] void DeadlySignalHandlerReentered(int sig) {
  const char* const name = SigToName(sig);
  write(STDERR_FILENO, "===REENTRY=== ", 14);
  write(STDERR_FILENO, name, 7);
  write(STDERR_FILENO, " ===REENTRY=== \n", 15);
  _exit(sig);
}

volatile int global_signal_depth = 0;

[[noreturn]] void DeadlySignalHandler(int sig, siginfo_t* info,
                                      void* ucontext) {
  constexpr int kMaxThreads = 1024;
  int tids[kMaxThreads];

  // Simple reentrancy guard.
  global_signal_depth = global_signal_depth + 1;
  if (global_signal_depth > 1) {
    DeadlySignalHandlerReentered(sig);
  }

  const int thread_count = EnumerateOtherThreads(tids, kMaxThreads);

  // Capture all other threads in the process so we can get a clean stacktrace.
  SignalThreads(tids, thread_count, kCaptureSignal);

  // Await threads entering the capture handler.
  AwaitCapturedThreads(thread_count);

  // Stackdump.
  const char* const name = SigToName(sig);
  write(STDERR_FILENO, "=== ", 4);
  write(STDERR_FILENO, name, 7);
  write(STDERR_FILENO, " ===\n", 5);
  stacktrace::DumpAllStacksWithLineNumbers(STDERR_FILENO, ucontext);

  _exit(sig);
}

void SetupDeadlySignalHandler() {
  struct sigaction s = {};
  s.sa_sigaction = &DeadlySignalHandler;
  sigemptyset(&s.sa_mask);
  s.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &s, nullptr);
  sigaction(SIGABRT, &s, nullptr);
  sigaction(SIGILL, &s, nullptr);
  sigaction(SIGBUS, &s, nullptr);
  sigaction(SIGFPE, &s, nullptr);
}

void SetupThreadCaptureHandler() {
  struct sigaction s = {};
  s.sa_handler = &CapturedThreadHandler;
  sigemptyset(&s.sa_mask);
  s.sa_flags = 0;
  sigaction(kCaptureSignal, &s, nullptr);
}

bool ValidateEnvironment() {
  // Invariant required by integer types that may union against pointers, such
  // as the codes libraries.
  const int min_addr =
      async_safe::ParseFileContentsAsDecimal("/proc/sys/vm/mmap_min_addr");
  if (min_addr < 65536) {
    std::cerr << "mondale.ftl requires mmap minimum address >= 65536; found ["
              << min_addr << "]" << std::endl;
    return false;
  }

  return true;
}

}  // namespace

void Initialize(int argc, char* argv[]) {
  if (!ValidateEnvironment()) {
    std::cerr << "Unsuitable environment. Exiting." << std::endl;
    exit(1);
  }

  SetupThreadCaptureHandler();
  SetupDeadlySignalHandler();
  // Future site of flag parsing.
}

}  // namespace base
