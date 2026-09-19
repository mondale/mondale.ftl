#include "base/rawlog.h"
#include "core/syscalls.h"
#include "testing/testing.h"

using ::testing::HasSubstr;
using ::testing::IsOk;
using namespace core;

namespace {

TEST(FStatAndRead) {
  auto fd = syscalls::Open("/tmp", O_TMPFILE | O_RDWR, 0).ValueOrDie();
  auto sb = syscalls::FStat(fd).ValueOrDie();
  EXPECT_EQ(static_cast<int>(sb.st_size), 0);
  EXPECT_EQ(size_t{0}, syscalls::Read(fd, nullptr, 100).ValueOrDie());  // eof
}

TEST(OpenAndStat) {
  auto result =
      syscalls::Open("/does/not/exist/probably", O_RDONLY, 0).result();
  EXPECT_EQ(BaseCode::kEnoent, result.base_code());
  EXPECT_THAT(syscalls::Stat("/does/not/exist/probably").result().ToString(),
              HasSubstr("Enoent"));
  EXPECT_THAT(syscalls::Access("/does/not/exist/probably", R_OK).ToString(),
              HasSubstr("Enoent"));
}

TEST(GetAndSetRlimit) {
  auto lims = syscalls::GetRLimit(RLIMIT_NOFILE).ValueOrDie();
  lims.rlim_cur = lims.rlim_max;
  EXPECT_THAT(syscalls::SetRLimit(RLIMIT_NOFILE, &lims), IsOk());
}

TEST(EventFdAndFcntl) {
  auto efd = syscalls::EventFd(0, EFD_NONBLOCK | EFD_CLOEXEC).ValueOrDie();
  EXPECT_THAT(syscalls::EventFdWrite(efd, 10), IsOk());

  EXPECT_EQ(10, syscalls::EventFdRead(efd).ValueOrDie());

  auto flags = syscalls::Fcntl(efd, F_GETFL).ValueOrDie();
  EXPECT_NE(flags & O_NONBLOCK, 0);
}

TEST(EpollPwait2AndCtl) {
  auto efd = syscalls::EventFd(0, EFD_NONBLOCK | EFD_CLOEXEC).ValueOrDie();
  auto epfd = syscalls::EpollCreate1(EPOLL_CLOEXEC).ValueOrDie();

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = efd.fd();
  EXPECT_THAT(syscalls::EpollCtl(epfd, EPOLL_CTL_ADD, efd, &ev), IsOk());

  EXPECT_THAT(syscalls::EventFdWrite(efd, 5), IsOk());

  struct epoll_event events[1];
  struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
  auto n =
      syscalls::EpollPwait2(epfd, events, 1, &timeout, nullptr).ValueOrDie();
  EXPECT_EQ(n, 1);
  EXPECT_EQ(events[0].data.fd, efd.fd());
  EXPECT_NE((events[0].events & EPOLLIN), 0);
}

}  // namespace
