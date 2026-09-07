#include "base/logging.h"
#include "base/process.h"
#include "capsule/generator.h"
#include "capsule/hashing.h"
#include "capsule/parser.h"
#include "core/file.h"
#include "core/vocabulary.h"

FLAG_COHORT(capsule_compiler);
FLAG(std::string, input_file_name, "");
FLAG(std::string, output_header_name, "");
FLAG(std::string, output_source_name, "");

namespace capsule {
namespace {

std::string ToHeader(const std::string& cc) {
  if (cc.ends_with(".cc")) {
    return cc.substr(0, cc.size() - 3) + ".h";
  }
  return cc;
}

Result ValidateInputFile(const std::string& f) {
  if (f.empty()) {
    return core::InvalidArgumentError(
        "No input file specified. Use "
        "--capsule_compiler.input_file_name=/path/to/file.capsule");
  }

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

Result ValidateHeaderFile(const std::string& f) {
  if (f.empty()) {
    return core::InvalidArgumentError(
        "No output header file specified. Use "
        "--capsule_compiler.output_header_name=/path/to/file.h");
  }
  return ValidateOutputFile(f);
}

Result ValidateSourceFile(const std::string& f) {
  if (f.empty()) {
    return core::InvalidArgumentError(
        "No output source file specified. Use "
        "--capsule_compiler.output_source_name=/path/to/file.cc");
  }
  return ValidateOutputFile(f);
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

Result ValidateFlags() {
  const std::string ifn = FLAG_LOOKUP(input_file_name);
  const std::string ohn = FLAG_LOOKUP(output_header_name);
  const std::string osn = FLAG_LOOKUP(output_source_name);
  Log(INFO) << "Using input file [" << ifn << "]";
  Log(INFO) << "Using output header file [" << ohn << "]";
  Log(INFO) << "Using output source file [" << osn << "]";
  TRY(ValidateInputFile(ifn));

  if (ohn.empty() && osn.empty()) {
    return core::InvalidArgumentError(
        "No output header or source file specified. Use at least one of "
        "--capsule_compiler.output_header_name=/path/to/file.h or "
        "--capsule_compiler.output_soruce_name=/path/to/file.cc");
  }
  if (!ohn.empty()) {
    TRY(ValidateHeaderFile(ohn));
  }
  if (!osn.empty()) {
    TRY(ValidateSourceFile(osn));
  }
  return Result::Ok();
}

}  // namespace

Result Compile() {
  TRY(ValidateFlags());

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
  if (!ohn.empty()) {
    TRY_ASSIGN(auto header, capsule::GenerateHeader(capfile));
    TRY(core::WriteContentsToFile(ohn, header));
  }
  if (!osn.empty()) {
    TRY_ASSIGN(auto source, capsule::GenerateSource(capfile, ToHeader(osn)));
    TRY(core::WriteContentsToFile(osn, source));
  }

  return Result::Ok();
}

}  // namespace capsule
