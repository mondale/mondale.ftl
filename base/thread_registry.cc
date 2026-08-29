#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstring>

#include "base/async_safe.h"
#include "base/thread_registry.h"

namespace base {
namespace {

std::atomic<ThreadRegistry*> g_thread_registry{nullptr};

}  // namespace

ThreadRegistry* ThreadRegistry::GetOrCreate() {
  ThreadRegistry* reg = g_thread_registry.load(std::memory_order_acquire);
  if (reg == nullptr) {
    static base::Mutex creation_mutex;
    base::MutexLock l(&creation_mutex);
    reg = g_thread_registry.load(std::memory_order_relaxed);
    if (reg == nullptr) {
      reg = new ThreadRegistry();
      g_thread_registry.store(reg, std::memory_order_release);
    }
  }
  return reg;
}

ThreadRegistry* ThreadRegistry::GetNoCreate() {
  return g_thread_registry.load(std::memory_order_acquire);
}

void ThreadRegistry::Add(pid_t t, std::string_view n) {
  base::MutexLock l(&mutex_);
  auto [it, inserted] = names_.try_emplace(t);
  FiniteName& fn = it->second;
  std::memset(fn, 0, kMaxNameLength);
  size_t len = std::min(n.size(), kMaxNameLength - 1);
  std::memcpy(fn, n.data(), len);
}

void ThreadRegistry::Remove(pid_t t) {
  base::MutexLock l(&mutex_);
  names_.erase(t);
}

bool ThreadRegistry::Lookup(pid_t t, char* buf, size_t len) {
  if (buf == nullptr || len == 0) {
    return false;
  }
  if (!mutex_.TryLock()) {
    return false;
  }
  auto it = names_.find(t);
  if (it == names_.end()) {
    mutex_.Unlock();
    return false;
  }
  size_t name_len = base::async_safe::StrLen(it->second);
  size_t copy_len = (name_len < len - 1) ? name_len : (len - 1);
  base::async_safe::MemCopy(buf, it->second, copy_len);
  buf[copy_len] = '\0';
  mutex_.Unlock();
  return true;
}

ScopedThreadRegistration::ScopedThreadRegistration(std::string_view name) {
  pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
  ThreadRegistry::GetOrCreate()->Add(tid, name);
}

ScopedThreadRegistration::~ScopedThreadRegistration() {
  pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
  if (auto* reg = ThreadRegistry::GetNoCreate()) {
    reg->Remove(tid);
  }
}

}  // namespace base
