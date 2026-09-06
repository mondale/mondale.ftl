#include "base/process.h"
#include "core/vocabulary.h"

FLAG_COHORT(compiler);
FLAG(std::string, input_file_name, "");
FLAG(std::string, output_header_name, "");
FLAG(std::string, output_source_name, "");

namespace {

Result ValidateInputFile(const std::string& f) {
  // Validate that the input file exists and is readable.
  return Result::Ok();
}

Result ValidateOutputFile(const std::string& f) {
  // Validate that the output file either doesnt' exist or exists and is
  // writable.
  return Result::Ok();
}

Result ValidateFlags() {
  TRY(ValidateInputFile(FLAG_LOOKUP(input_file_name)));
  TRY(ValidateOutputFile(FLAG_LOOKUP(output_header_name)));
  TRY(ValidateOutputFile(FLAG_LOOKUP(output_source_name)));
  return Result::Ok();
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  CHECK_OK(ValidateFlags());
  return EXIT_SUCCESS;
}
