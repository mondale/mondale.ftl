#include <execinfo.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <list>

#include "base/async_safe.h"
#include "base/process.h"
#include "base/raw_syscalls.h"
#include "base/stacktrace.h"

using namespace base::raw_syscalls;

namespace base {
namespace {

bool IsLogDirectoryWritable() {
  // mkstemp requires a modifiable character array template
  char temp_path[] = "/var/log/mondale/directory_probe_XXXXXX";
  const int fd = mkstemp(temp_path);
  if (fd < 0) {
    return false;
  }

  close(fd);
  ::unlink(temp_path);
  return true;
}

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
    case SIGTERM:
      return "SIGTERM";
    case SIGINT:
      return "SIGINT";
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
  sigaction(SIGINT, &s, nullptr);
  sigaction(SIGTERM, &s, nullptr);
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

  // Require that the /var/log/mondale directory exists and accomodates writes.
  if (!IsLogDirectoryWritable()) {
    std::cerr << "[/var/log/mondale/] does not exist or is not writeable."
              << std::endl;
  }

  return true;
}

std::list<std::function<void()>>* global_startup_hooks = nullptr;
std::list<std::function<void()>>* GetStartupHooks() {
  if (nullptr == global_startup_hooks) {
    global_startup_hooks = new std::list<std::function<void()>>();
  }
  return global_startup_hooks;
}

void RunStartupHooks() {
  auto& l = *GetStartupHooks();
  for (auto& fn : l) {
    fn();
  }
  delete global_startup_hooks;
  global_startup_hooks = nullptr;
}

}  // namespace

bool RegisterStartupHook(std::function<void()> fn) {
  GetStartupHooks()->push_back(std::move(fn));
  return true;
}

void Initialize(int argc, char* argv[]) {
  if (!ValidateEnvironment()) {
    std::cerr << "Unsuitable environment. Exiting." << std::endl;
    exit(1);
  }

  SetupThreadCaptureHandler();
  SetupDeadlySignalHandler();
  // Future site of flag parsing.

  RunStartupHooks();
}

}  // namespace base
