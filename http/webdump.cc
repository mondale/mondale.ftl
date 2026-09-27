#include "base/process.h"
#include "core/vocabulary.h"

namespace {

Result Webdump() { return Result::Ok(); }

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  core::util::DieElegantlyIfNotOk(Webdump());
  return EXIT_SUCCESS;
}
