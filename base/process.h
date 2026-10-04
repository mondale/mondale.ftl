#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

#include <functional>

#include "base/time.h"

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

// Returns true (probably) when you're (probably) in a forked subprocess.
bool AmIInAForkedSubprocess();

// Blocks until CTRLC+.
void AwaitSigInt();

// Returns the WallTime at which the process began. Well, Initialize()d. What
// do you want from me anyway?
WallTime ProcessStartTime();

// Return the uptime of the process.
Duration ProcessUptime();

}  // namespace base

#endif  // #ifndef BASE_PROCESS_H_
