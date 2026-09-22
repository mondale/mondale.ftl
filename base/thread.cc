#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "base/cpu.h"
#include "base/flags.h"
#include "base/rawlog.h"
#include "base/thread.h"
#include "base/thread_registry.h"

FLAG_COHORT(thread);
FLAG(int, foreground_cpus, -1).Ge(-1).Lt(1000);
FLAG(int, background_cpus, -1).Ge(-1).Lt(1000);

namespace base {
namespace internal {
namespace {

int ForegroundCpuCount() {
  const auto n = NumCpus();
  if (n <= 2) return 1;
  int f = FLAG_LOOKUP(foreground_cpus);
  if (-1 == f) f = n - 1;
  return std::clamp(f, 1, n - 1);
}

int BackgroundCpuCount() {
  const auto n = NumCpus();
  if (n <= 2) return 1;
  int f = FLAG_LOOKUP(background_cpus);
  if (-1 == f) f = 1;
  return std::clamp(f, 1, n - 1);
}

std::string CpuSetToString(const cpu_set_t& mask) {
  std::ostringstream oss;
  bool first = true;

  for (int i = 0; i < CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &mask)) {
      if (!first) {
        oss << ", ";
      }
      oss << i;
      first = false;
    }
  }
  return oss.str();
}

class ThreadingStrategy final {
 public:
  static ThreadingStrategy* Instance() {
    static ThreadingStrategy* ts = new ThreadingStrategy();
    return ts;
  }

  ThreadingStrategy() {
    CPU_ZERO(&foreground_mask_);
    CPU_ZERO(&background_mask_);

    const auto n = NumCpus();
    const auto f_configured = ForegroundCpuCount();
    const auto b_configured = BackgroundCpuCount();
    int f = f_configured;
    int b = b_configured;

    if ((f + b) > n) {
      b = 1;
      f = n - 1;
      if (f <= 0) f = 1;
    }

    if (f != f_configured || b != b_configured) {
      RAW_WARNING << "Invalid threading configuration. NumCpus[" << n
                  << "] ForegroundCpus[" << f_configured << "] BackgroundCpus["
                  << b_configured << "]. Reconfigured to ForegroundCpus[" << f
                  << "] BackgroundCpus[" << b << "].";
    }

    // Special-case 1 CCPU.
    if (n == 1) {
      CPU_SET(0, &foreground_mask_);
      CPU_SET(0, &background_mask_);
    } else {
      // Build the background mask from the lower CPUs and the foreground mask
      // from the upper CPUs.
      int bg = 0;
      for (int c = 0; c < (f + b); ++c) {
        if (bg < b) {
          CPU_SET(c, &background_mask_);
          ++bg;
        } else {
          CPU_SET(c, &foreground_mask_);
        }
      }
    }
  }

  void BecomeBackgroundThread() { Become(&background_mask_); }

  void BecomeForegroundThread() { Become(&foreground_mask_); }

 private:
  void Become(cpu_set_t* cs) {
    const int ret = sched_setaffinity(GetCachedTid(), sizeof(*cs), cs);
    if (ret < 0) {
      RAW_WARNING << "Thread " << GetCachedTid()
                  << " could not join designated CPU set ["
                  << CpuSetToString(*cs) << "].";
    }
  }
  cpu_set_t foreground_mask_;
  cpu_set_t background_mask_;
};

void ThreadStart(std::string name, std::function<void()> fn) {
  pthread_setname_np(pthread_self(), name.c_str());
  ScopedThreadRegistration str(name);
  fn();
}

}  // namespace

class ThreadImpl final {
 public:
  ThreadImpl() = default;
  ~ThreadImpl() { Join(); }

  void Join() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  void Start(std::string_view np, std::function<void()> fn) {
    thread_ = std::thread(
        [n = std::string(np), f = std::move(fn), a = &ready_to_join_]() {
          ThreadStart(n, std::move(f));
          a->store(true, std::memory_order_release);
        });
  }

  bool ReadyToJoin() const {
    return ready_to_join_.load(std::memory_order_acquire);
  }

 private:
  std::thread thread_;
  std::atomic<bool> ready_to_join_{false};
};

}  // namespace internal

Thread::Thread(std::unique_ptr<internal::ThreadImpl> impl)
    : impl_(std::move(impl)) {}
Thread::~Thread() { Join(); }
void Thread::Join() { impl_->Join(); }
bool Thread::ReadyToJoin() const { return impl_->ReadyToJoin(); }

std::unique_ptr<Thread> CreateThread(std::string_view np,
                                     std::function<void()> fn) {
  auto impl = std::make_unique<internal::ThreadImpl>();
  impl->Start(np, std::move(fn));
  return std::make_unique<Thread>(std::move(impl));
}

std::unique_ptr<Thread> CreateThread(std::function<void()> fn) {
  return CreateThread("", fn);
}

void CreateDetachedThread(std::string_view np, std::function<void()> fn) {
  std::string n = std::string(np);
  std::thread t([n = std::move(n), fn = std::move(fn)]() {
    internal::ThreadStart(n, fn);
  });
  t.detach();
}

void CreateDetachedThread(std::function<void()> fn) {
  CreateDetachedThread("", fn);
}

pid_t GetTid() { return syscall(SYS_gettid); }

void BecomeBackgroundThread() {
  internal::ThreadingStrategy::Instance()->BecomeBackgroundThread();
}

void BecomeForegroundThread() {
  internal::ThreadingStrategy::Instance()->BecomeForegroundThread();
}

}  // namespace base
