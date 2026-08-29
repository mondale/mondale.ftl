#ifndef BASE_THREAD_REGISTRY_H_
#define BASE_THREAD_REGISTRY_H_

#include <string_view>
#include <unordered_map>

#include "base/mutex.h"

namespace base {

class ThreadRegistry final {
 public:
  static ThreadRegistry* GetOrCreate();
  static ThreadRegistry* GetNoCreate();

  void Add(pid_t t, std::string_view n);
  void Remove(pid_t t);

  // Returns true when t is found and buf is populated. May return false due to
  // contention. Async signal safe due to trylock.
  bool Lookup(pid_t t, char* buf, size_t len);

 private:
  static constexpr size_t kMaxNameLength = 256;
  using FiniteName = char[kMaxNameLength];
  ThreadRegistry() = default;

  base::Mutex mutex_;
  std::unordered_map<pid_t, FiniteName> names_ GUARDED_BY(mutex_);
};

class ScopedThreadRegistration final {
 public:
  ScopedThreadRegistration(std::string_view name);
  ~ScopedThreadRegistration();

 private:
};

}  // namespace base

#endif  // #ifndef BASE_THREAD_REGISTRY_H_
