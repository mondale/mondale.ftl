#include "base/process.h"
#include "capsule/compiler.h"
#include "core/vocabulary.h"

namespace {

void DieElegantlyIfNotOk(Result r) {
  if (IsOk(r)) return;
  Log(ERROR) << r;
  std::cerr << r << std::endl;
  base::FlushLogs();
  exit(1);
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  DieElegantlyIfNotOk(capsule::Compile());
  return EXIT_SUCCESS;
}
