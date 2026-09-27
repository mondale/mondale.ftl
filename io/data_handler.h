#ifndef IO_DATA_HANDLER_H_
#define IO_DATA_HANDLER_H_

#include <memory>
#include <queue>

#include "core/small_map.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"
#include "io/sink_buffer.h"
#include "io/source_buffer.h"

namespace io {

// Generic IoHandler for a data FD type that never wants a partial read.
//
// Subclasses may partially override.
class DataHandler : public IoHandler,
                    public std::enable_shared_from_this<DataHandler> {
 private:
  struct PrivateTag {};

 public:
  explicit DataHandler(PrivateTag) {}
  virtual ~DataHandler() = default;

  static std::shared_ptr<DataHandler> Create() {
    return std::make_shared<DataHandler>(PrivateTag{});
  }

  ResultOr<Outcome> HandleRead(FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(FdHandle h,
                                const core::FileDescriptor& fd) override;

  void Post(FdHandle h, SourceBuffer sb);
  void Post(FdHandle h, SinkBuffer sb);

  ResultOr<FdHandle> Add(core::FileDescriptor fd);

 private:
  struct PerFd {
    std::queue<SourceBuffer> sources;
    std::queue<SinkBuffer> sinks;
  };
  core::SmallMap<FdHandle, PerFd> perfd_;
};

}  // namespace io

#endif  // #ifndef IO_DATA_HANDLER_H_
