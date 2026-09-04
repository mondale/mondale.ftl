#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

#include <functional>

namespace base {

// Initialize process-wide common behaviors.
void Initialize(int argc, char* argv[]);

// Request a startup hook. Safe to call before main.
// Just hold onto the return value for good luck.
bool RegisterStartupHook(std::function<void()> fn);

// Request to register the logs flushing hook.
void RegisterLogsFlushHook(std::function<void()> fn);

// Request to flush logs.
void FlushLogs();

}  // namespace base

#endif  // #ifndef BASE_PROCESS_H_
