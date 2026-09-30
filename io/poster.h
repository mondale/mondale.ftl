#ifndef IO_POSTER_H_
#define IO_POSTER_H_

#include "core/vocabulary.h"
#include "io/sink_buffer.h"
#include "io/source_buffer.h"

namespace io {

class Poster {
 public:
  virtual void Post(io::SourceBuffer sb) = 0;
  virtual void Post(io::SinkBuffer sb) = 0;
  virtual void PostClose() = 0;

 private:
};

}  // namespace io

#endif  // #ifndef IO_POSTER_H_
