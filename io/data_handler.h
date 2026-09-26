#ifndef IO_DATA_HANDLER_H_
#define IO_DATA_HANDLER_H_

#include <queue>

#include "core/small_map.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"
#include "io/sink_buffer.h"
#include "io/source_buffer.h"

namespace io {

// Generic IoHandler for a data FD type that never wants a partial read.
class DataHandler final : public IoHandler {
 public:
  ResultOr<Outcome> HandleRead(FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(FdHandle h,
                                const core::FileDescriptor& fd) override;

  void Post(FdHandle h, SourceBuffer sb);
  void Post(FdHandle h, SinkBuffer sb);

 private:
  struct PerFd {
    std::queue<SourceBuffer> sources;
    std::queue<SinkBuffer> sinks;
  };
  core::SmallMap<FdHandle, PerFd> perfd_;
};

}  // namespace io

#endif  // #ifndef IO_DATA_HANDLER_H_
