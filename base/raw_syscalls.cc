#include <fcntl.h>
#include <sys/syscall.h>

#include "base/raw_syscalls.h"

namespace base::raw_syscalls {
namespace {

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

}  // namespace

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
long SysFutex(void* addr, int op) {
  return Sys(__NR_futex, reinterpret_cast<long>(addr), op);
}
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
long SysSchedYield() { return Sys(__NR_sched_yield); }
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
long SysTKill(int tid, int sig) { return Sys(__NR_tkill, tid, sig); }
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

}  // namespace base::raw_syscalls
