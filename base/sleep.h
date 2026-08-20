#ifndef BASE_SLEEP_H_
#define BASE_SLEEP_H_

#include "base/time.h"

namespace base {

// Sleeps the calling thread for the specified duration or until the specified
// time is reached. Very small sleeps may be serviced by spins.
void SleepFor(Duration d);
void SleepUntil(WallTime w);
void SleepUntil(MonotonicTime m);
void SleepUntil(CycleTime c);

}  // namespace base

#endif  // #ifndef BASE_SLEEP_H_
