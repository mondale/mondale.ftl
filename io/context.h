#ifndef IO_CONTEXT_H_
#define IO_CONTEXT_H_

#include "base/time.h"
#include "io/io_handler.h"

namespace io {

class Context {
 public:
  static Context* Current() { return tl_current_; }
  static void SetCurrent(Context* c) { tl_current_ = c; }

  Context(base::MonotonicTime ls) : loop_start_(ls) {}

  virtual void RequestRead(FdHandle h) = 0;
  virtual void RequestWrite(FdHandle h) = 0;
  virtual void Run(FdHandle h, std::move_only_function<void()> fn) = 0;
  virtual ResultOr<FdHandle> Add(std::shared_ptr<IoHandler> h,
                                 core::FileDescriptor fd) = 0;
  virtual void Eject(FdHandle h) = 0;

  base::MonotonicTime loop_start() const { return loop_start_; }

 private:
  const base::MonotonicTime loop_start_;

  static thread_local Context* tl_current_;
};

}  // namespace io

#endif  // #ifndef IO_CONTEXT_H_
