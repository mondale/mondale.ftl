#ifndef BASE_RAW_SYSCALLS_H_
#define BASE_RAW_SYSCALLS_H_

#include <poll.h>
#include <signal.h>
#include <stdint.h>

// Raw system calls with no intervening libc. All async-signal safe.

namespace base::raw_syscalls {

// Raw system calls, no libc.
long SysRead(int fd, void* buf, size_t n);
long SysWrite(int fd, const void* buf, size_t n);
long SysOpen(const char* path, int flags);
long SysClose(int fd);
long SysFutex(void* addr, int op);
long SysGetdents64(int fd, void* buf, size_t n);
long SysReadlink(const char* path, char* buf, size_t n);
long SysPipe2(int fds[2], int flags);
long SysDup3(int old_fd, int new_fd, int flags);
long SysExecve(const char* path, char* const argv[], char* const envp[]);
[[noreturn]] void SysExit(int code);
long SysWait4(int pid, int* status, int options);
long SysKill(int pid, int sig);
long SysGetpid();
long SysGettid();
long SysPoll(struct pollfd* fds, unsigned n, int timeout_ms);
long SysProcessVmReadv(int pid, const struct iovec* local, unsigned long lcnt,
                       const struct iovec* remote, unsigned long rcnt);
long SysSigprocmask(int how, const uint64_t* set, uint64_t* oldset);
long SysSigtimedwait(const uint64_t* set, const struct timespec* ts);
long SysTKill(int tid, int sig);
long SysSchedYield();

// Helpers.
void SleepMicros(long usec);
long SafeFork();

}  // namespace base::raw_syscalls

#endif  // #ifndef BASE_RAW_SYSCALLS_H_
