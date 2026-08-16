#ifndef BASE_STACKTRACE_H_
#define BASE_STACKTRACE_H_

namespace base::stacktrace {

void DumpAllStacksWithLineNumbers(int fd);
void DumpAllStacksWithLineNumbers(int fd, const void* ucontext);

}  // namespace base::stacktrace

#endif  // #ifndef BASE_STACKTRACE_H_
