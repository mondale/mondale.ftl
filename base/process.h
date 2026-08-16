#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

namespace base {

// Initialize process-wide common behaviors.
void Initialize(int argc, char* argv[]);

// Dumps the caller's stacktrace, async signal safe.
void DumpStackTrace(int fd);

}  // namespace base

#endif  // #ifndef BASE_PROCESS_H_
