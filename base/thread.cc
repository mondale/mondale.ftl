#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "base/thread.h"

namespace base {
namespace internal {
namespace {

void ThreadStart(std::string name, std::function<void()> fn) {
  pthread_setname_np(pthread_self(), name.c_str());
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

void CreateDetachedThread(std::string_view np, std::function<void()> fn) {
  std::string n = std::string(np);
  std::thread t([n = std::move(n), fn = std::move(fn)]() {
    internal::ThreadStart(n, fn);
  });
  t.detach();
}

pid_t GetTid() { return syscall(SYS_gettid); }

}  // namespace base
