#include "io/io_handler.h"

namespace io {

std::string ToString(IoHandler::Outcome o) {
  switch (o) {
    case IoHandler::Outcome::kFdEagain:
      return "kFdEagain";
    case IoHandler::Outcome::kYield:
      return "kYield";
    case IoHandler::Outcome::kSuspend:
      return "kSuspend";
    case IoHandler::Outcome::kClose:
      return "kClose";
  }
}

std::ostream& operator<<(std::ostream& out, IoHandler::Outcome o) {
  out << ToString(o);
  return out;
}

}  // namespace io
