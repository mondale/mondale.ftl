#ifndef IO_CONTEXT_H_
#define IO_CONTEXT_H_

#include "base/time.h"
#include "io/io_handler.h"

namespace io {

class Context {
 public:
  Context(base::MonotonicTime ls) : loop_start_(ls) {}

  virtual void RequestRead(FdHandle h) = 0;
  virtual void RequestWrite(FdHandle h) = 0;
  virtual void Run(FdHandle h, std::move_only_function<void(Context*)> fn) = 0;

  base::MonotonicTime loop_start() const { return loop_start_; }

 private:
  const base::MonotonicTime loop_start_;
};

}  // namespace io

#endif  // #ifndef IO_CONTEXT_H_
