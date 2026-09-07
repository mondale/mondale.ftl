#include "base/process.h"
#include "capsule/compiler.h"
#include "core/vocabulary.h"

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  core::util::DieElegantlyIfNotOk(capsule::Compile());
  return EXIT_SUCCESS;
}
