#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/basic_test.h"
#include "base/raw_syscalls.h"

namespace {

TEST(GetpidAndGettid) {
  const long pid = base::raw_syscalls::SysGetpid();
  const long tid = base::raw_syscalls::SysGettid();

  EXPECT_GT(pid, 0L);
  EXPECT_GT(tid, 0L);
  EXPECT_EQ(static_cast<long>(getpid()), pid);
}

TEST(SchedYieldAndSleep) {
  EXPECT_EQ(0L, base::raw_syscalls::SysSchedYield());

  // Simple smoke test for sleep helper
  base::raw_syscalls::SleepMicros(1000);
}

TEST(PipeWriteReadClose) {
  int fds[2] = {-1, -1};
  ASSERT_EQ(0L, base::raw_syscalls::SysPipe2(fds, 0));
  ASSERT_GT(fds[0], 0);
  ASSERT_GT(fds[1], 0);

  const char msg[] = "raw_syscalls_test";
  const long msg_len = static_cast<long>(sizeof(msg) - 1);

  const long bytes_written = base::raw_syscalls::SysWrite(fds[1], msg, msg_len);
  EXPECT_EQ(msg_len, bytes_written);

  char buf[32] = {0};
  const long bytes_read = base::raw_syscalls::SysRead(fds[0], buf, sizeof(buf));
  EXPECT_EQ(msg_len, bytes_read);

  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[0]));
  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[1]));
}

TEST(OpenAndReadlink) {
  const long fd = base::raw_syscalls::SysOpen("/dev/null", O_RDONLY);
  ASSERT_GT(fd, 0L);
  EXPECT_EQ(0L, base::raw_syscalls::SysClose(static_cast<int>(fd)));

  char buf[256] = {0};
  const long res =
      base::raw_syscalls::SysReadlink("/proc/self/exe", buf, sizeof(buf));
  EXPECT_GT(res, 0L);
}

TEST(Dup3) {
  int fds[2] = {-1, -1};
  ASSERT_EQ(0L, base::raw_syscalls::SysPipe2(fds, 0));

  const int target_fd = 99;
  const long dup_res = base::raw_syscalls::SysDup3(fds[0], target_fd, 0);
  EXPECT_EQ(static_cast<long>(target_fd), dup_res);

  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[0]));
  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[1]));
  EXPECT_EQ(0L, base::raw_syscalls::SysClose(target_fd));
}

TEST(Poll) {
  int fds[2] = {-1, -1};
  ASSERT_EQ(0L, base::raw_syscalls::SysPipe2(fds, 0));

  struct pollfd pfd;
  pfd.fd = fds[0];
  pfd.events = POLLIN;
  pfd.revents = 0;

  // Nothing to read yet, timeout 0 should return 0
  EXPECT_EQ(0L, base::raw_syscalls::SysPoll(&pfd, 1, 0));

  // Write a byte and check again
  EXPECT_EQ(1L, base::raw_syscalls::SysWrite(fds[1], "x", 1));
  EXPECT_EQ(1L, base::raw_syscalls::SysPoll(&pfd, 1, 0));
  EXPECT_TRUE((pfd.revents & POLLIN) != 0);

  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[0]));
  EXPECT_EQ(0L, base::raw_syscalls::SysClose(fds[1]));
}

TEST(ForkWaitExit) {
  const long pid = base::raw_syscalls::SafeFork();
  ASSERT_GE(pid, 0L);

  if (pid == 0) {
    // Child process
    base::raw_syscalls::SysExit(42);
  }

  // Parent process
  int status = 0;
  const long wait_res =
      base::raw_syscalls::SysWait4(static_cast<int>(pid), &status, 0);
  EXPECT_EQ(pid, wait_res);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(42, WEXITSTATUS(status));
}

TEST(ProcessVmReadv) {
  const char src[] = "vm_read_test";
  char dst[32] = {0};

  struct iovec local;
  local.iov_base = dst;
  local.iov_len = sizeof(src);

  struct iovec remote;
  remote.iov_base = const_cast<char*>(src);
  remote.iov_len = sizeof(src);

  const long pid = base::raw_syscalls::SysGetpid();
  const long read_bytes = base::raw_syscalls::SysProcessVmReadv(
      static_cast<int>(pid), &local, 1, &remote, 1);

  EXPECT_EQ(static_cast<long>(sizeof(src)), read_bytes);
}

TEST(Sigprocmask) {
  uint64_t oldset = 0;
  // Fetch current signal mask without modifying it
  const long res =
      base::raw_syscalls::SysSigprocmask(SIG_SETMASK, nullptr, &oldset);
  EXPECT_EQ(0L, res);
}

}  // namespace
