#include "io/context.h"

namespace io {

thread_local Context* Context::tl_current_ = nullptr;

}
