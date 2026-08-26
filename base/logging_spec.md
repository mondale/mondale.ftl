# C++ Async Logging Library Specification

## 1. Interface & Core Abstractions

* **Source Location:** Captures call-site metadata using `base::source_location`.
* **Log Levels:** `INFO`, `WARNING`, `ERROR`, `FATAL`, and `DFATAL`.
  * `FATAL`: Permanently closes admission of new log entries, drains all queued log entries across all worker queues, flushes output streams, and executes `raise(SIGABRT)`.
  * `DFATAL`: Evaluates as `FATAL` in debug builds (`!NDEBUG`) and `ERROR` in optimized builds (`NDEBUG`).
* **Thread Identification:** Identifies calling threads using `base::GetCachedTid()`.
* **Header Format:** Formatted as:
  `[Severity Initial][TID] [HH:MM] [MM/DD/YYYY] [Filename]:[Line]] [Message]`
  *(Example: `I8134 17:11 8/25/2026 foo.cc:71] Hello world!`)*
* **Stream Destination & Multiplexing:**
  * Each logging severity maintains an independent output destination stream.
  * Destinations can be independently routed to specific file paths or multiplexed onto standard sinks (`stdout` / `stderr`).
* **Stream Formatting Isolation:** Stream formatting state (e.g., `std::hex`, `std::setw`, fill characters) is strictly reset back to default standard settings after every individual log statement.

## 2. Logging Syntaxes & Modifiers

* **Basic Logging:** Invoked via `Log(LEVEL) << message;`.
* **Stateful Modifiers:** Extended syntax accepting thread-safe call-site predicates:
  * `Log(INFO, First(N))`: Logs only the first `N` executions at that call site.
  * `Log(INFO, SampleOneIn(N))`: Logs 1 out of every `N` executions via a countdown sampler.
  * *Extensibility:* Custom stateful modifiers must be supported without modifying core library code.
* **Verbosity Logging (`VLOG`):**
  * `VLOG(n)`: Evaluates if global verbosity is >= n OR if the current file matches an active `vmodule` rule at level >= n.
  * `DVLOG(n)`: Evaluates `VLOG(n)` in debug builds; no-op in optimized builds.
* **Module Filtering (`vmodule`):**
  * Exact stem-matching string rules (no glob support). A rule for `foo` matches both `foo.cc` and `foo.h`.
* **Assertion Checks (`CHECK`):**
  * `CHECK(condition)`: Triggers a `FATAL` log if the condition evaluates to `false`.
  * `CHECK_[EQ|NE|LE|LT|GE|GT](val1, val2)`: Evaluates operands once, captures symbol names, streams evaluated values of both sides if the check fails (e.g., `Check failed: x < foo() (10 vs 5)`), and triggers `FATAL`.
  * `CHECK_OK(status)`: Evaluates a status object; triggers `FATAL` with the status error string if not OK.
  * `DCHECK_*`: Debug-only variants; compiled out in optimized builds (`NDEBUG`).
  * *Formatting Requirement:* Types streamed into `CHECK` macros or `Log` statements must implement `operator<<`. Absence of `operator<<` causes a compile-time error.

## 3. Performance, Memory & Queue Architecture

* **Zero-Allocation Hot Path:** Logging string literals and primitive types must make zero heap allocations on the calling thread.
* **Non-Blocking & Buffering Strategy:**
  * Log entry processing and disk I/O are offloaded to a dedicated background processing thread.
  * Buffer capacity limit is configured on a **per-queue** basis (enforced approximately).
  * *Default Limit:* **8 megabytes** per queue.
  * *Overflow Policy:* If a queue's byte limit is exceeded, calling threads targeting that queue drop incoming log messages immediately without blocking.
  * *Recovery Accounting:* The background worker tracks dropped log counts and automatically emits an `ERROR` log indicating the total number of dropped entries as soon as queues unclog.
* **Queue Topology & Migration Mechanics:**
  * The inter-thread queue topology between producer threads and the background logging thread is configurable:
    1. Single global queue (`1`).
    2. One queue per CPU core (`NumCpus()`).
    3. One queue per NUMA domain (`NumNumaDomains()`).
  * *Default:* One queue per NUMA domain.
  * *System Helpers:* Topology utilizes `NumCpus()`, `GetCurrentCpu()`, `NumNumaDomains()`, and `NumaDomainFor(int cpu_number)`.
  * *Thread Safety & Migration:* All queue tails are thread-safe. Thread migration across CPUs/NUMA domains between log construction and dispatch is tolerated as a transient performance impact; correctness is preserved.

## 4. Global Configuration & Lifecycle API

* `SetLoggingVerbosity(int level)` *(Default: `0` / off)*
* `SetVmodules(const std::vector<std::string>& module_names)`
* `SetLogDestination(LogLevel level, const std::string& filepath_or_stream)`
* `SetQueueTopology(QueueTopology topology)`
* `SetPerQueueBufferLimit(size_t max_bytes)`
* **Shutdown Lifecycle:**
  * Registers an `atexit` handler to flush all active queue buffers to disk during normal process exit.
  * Any `Log` statements issued during or after `atexit` handler execution are silently dropped.
