#ifndef CORE_SYSCALLS_H_
#define CORE_SYSCALLS_H_

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <string_view>
#include <utility>

#include "core/file_descriptor.h"
#include "core/result.h"

namespace core::syscalls {

Result Access(std::string_view path, int mode);
ResultOr<FileDescriptor> EpollCreate1(int flags);
Result EpollCtl(const FileDescriptor& epfd, int op, const FileDescriptor& fd,
                struct epoll_event* event);
ResultOr<int> EpollPwait2(const FileDescriptor& epfd,
                          struct epoll_event* events, int maxevents,
                          const struct timespec* timeout,
                          const sigset_t* sigmask);
ResultOr<FileDescriptor> EventFd(unsigned int initval, int flags);
ResultOr<uint64_t> EventFdRead(const FileDescriptor& fd);
Result EventFdWrite(const FileDescriptor& fd, eventfd_t value);
ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd);
ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd, int arg);
ResultOr<int> Fcntl(const FileDescriptor& fd, int cmd, void* arg);
ResultOr<struct stat> FStat(const FileDescriptor& fd);
ResultOr<struct rlimit> GetRLimit(int resource);
Result Madvise(void* p, size_t n, int advice);
ResultOr<FileDescriptor> Open(std::string_view path, int flags, mode_t mode);
ResultOr<std::pair<FileDescriptor, FileDescriptor>> Pipe2(int flags);
ResultOr<size_t> Read(const FileDescriptor& fd, char* buf, size_t count);
Result SetRLimit(int resource, const struct rlimit* l);
ResultOr<std::pair<FileDescriptor, FileDescriptor>> SocketPair(int domain,
                                                               int type,
                                                               int protocol);
ResultOr<struct stat> Stat(std::string_view path);
ResultOr<size_t> Write(const FileDescriptor& fd, const char* buf, size_t count);

}  // namespace core::syscalls

#endif  // #ifndef CORE_SYSCALLS_H_
