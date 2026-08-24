#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_

#include <memory>
#include <string_view>

namespace base {
namespace internal {
class ThreadImpl;
}  // namespace internal

class Thread final {
 public:
  explicit Thread(std::unique_ptr<internal::ThreadImpl> impl);

  // Thread's dtor blocks until the underlying thread has completed execution.
  ~Thread();

  // Blocks caller until this thread has finished executing its target function.
  //
  // May be called repeatedly. Idempotent.
  void Join();

  // Returns true when Join() is done. May be polled as in indicator of
  // deletion readiness.
  bool ReadyToJoin() const;

 private:
  std::unique_ptr<internal::ThreadImpl> impl_;
};

std::unique_ptr<Thread> CreateThread(std::string_view name_prefix,
                                     std::function<void()> fn);
void CreateDetachedThread(std::string_view name_prefix,
                          std::function<void()> fn);

}  // namespace base

#endif  // #ifndef BASE_THREAD_H_
