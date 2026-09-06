#include "base/logging.h"
#include "base/process.h"
#include "capsule/generator.h"
#include "capsule/hashing.h"
#include "capsule/parser.h"
#include "core/file.h"
#include "core/vocabulary.h"

FLAG_COHORT(compiler);
FLAG(std::string, input_file_name, "");
FLAG(std::string, output_header_name, "");
FLAG(std::string, output_source_name, "");

namespace {

Result ValidateInputFile(const std::string& f) {
  // Validate that the input file exists and is readable.
  if (!core::FileExists(f)) {
    return core::NotFoundError(
        strings::Format("Capsule input file [{}] not found.", f));
  }

  if (!core::FileIsReadable(f)) {
    return core::PermissionError(
        strings::Format("Capsule input file [{}] not readable.", f));
  }

  return Result::Ok();
}

Result ValidateOutputFile(const std::string& f) {
  // Validate that the output file either doesn't exist or exists and is
  // writable.
  if (!core::FileExists(f)) {
    return Result::Ok();
  }
  if (core::FileIsWriteable(f)) {
    return Result::Ok();
  }
  return core::PermissionError(strings::Format(
      "Capsule output file [{}] exists and is not writeable.", f));
}

Result ValidateFlags() {
  const std::string ifn = FLAG_LOOKUP(input_file_name);
  const std::string ohn = FLAG_LOOKUP(output_header_name);
  const std::string osn = FLAG_LOOKUP(output_source_name);
  TRY(ValidateInputFile(ifn));
  TRY(ValidateOutputFile(ohn));
  TRY(ValidateOutputFile(osn));
  return Result::Ok();
}

ResultOr<std::string> ReadInputFile() {
  const std::string ifn = FLAG_LOOKUP(input_file_name);
  return core::ReadContentsFromFile(ifn);
}

ResultOr<capsule::CapsuleFile> Parse(std::string_view contents) {
  const std::string ifn = FLAG_LOOKUP(input_file_name);
  capsule::Parser p(contents, ifn);
  return p.Parse();
}

Result Compile() {
  // Read input stream.
  TRY_ASSIGN(auto contents, ReadInputFile());

  // Parse.
  TRY_ASSIGN(auto capfile, Parse(contents));

  // Annotate and check for errors.
  TRY(capsule::ComputeAndValidateHashes(&capfile));

  // Generate header and soruce.Note that source needs to know the header's
  // name.
  const std::string ohn = FLAG_LOOKUP(output_header_name);
  const std::string osn = FLAG_LOOKUP(output_source_name);
  TRY_ASSIGN(auto header, capsule::GenerateHeader(capfile));
  TRY_ASSIGN(auto source, capsule::GenerateSource(capfile, ohn));

  // Write to output files.
  TRY(core::WriteContentsToFile(ohn, header));
  TRY(core::WriteContentsToFile(osn, source));

  return Result::Ok();
}

void DieElegantlyIfNotOk(Result r) {
  if (IsOk(r)) return;
  Log(ERROR) << r;
  std::cerr << r << std::endl;
  exit(1);
}

}  // namespace

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv);
  DieElegantlyIfNotOk(ValidateFlags());
  DieElegantlyIfNotOk(Compile());
  return EXIT_SUCCESS;
}
