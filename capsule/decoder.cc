#include "capsule/decoder.h"

namespace capsule {
namespace {

Result ErrorLengthDecoding(size_t memory_length, uint32_t encoded_length) {
  return Result(Code::kError,
                strings::Format("Capsule decoding encountered encoded length "
                                "[{}] in excess of memory length [{}].",
                                encoded_length, memory_length));
}

Result ErrorImpliedLengthDecoding(size_t implied_length,
                                  uint32_t encoded_length) {
  return Result(Code::kError,
                strings::Format("Capsule decoding encountered implied length "
                                "[{}] in excess of encoded length [{}].",
                                implied_length, encoded_length));
}

Result ErrorInsufficientMemoryLength(size_t memory_length) {
  return Result(
      Code::kError,
      strings::Format("Cannot decode a capsule from [{}] bytes; 8 required.",
                      memory_length));
}

}  // namespace

Decoder::Decoder(const void* base) : base_(base), length_(0), vm_() {}

// static
ResultOr<Decoder> Decoder::Build(const void* base, size_t memory_length) {
  if (memory_length < sizeof(abi::Header)) {
    return ErrorInsufficientMemoryLength(memory_length);
  }
  const auto* const header = reinterpret_cast<const abi::Header*>(base);
  const uint32_t otes = header->offset_table_count;
  const uint32_t length = header->capsule_length;
  if (length > memory_length) {
    return ErrorLengthDecoding(memory_length, length);
  }

  const uint32_t implied_length =
      otes * sizeof(abi::OffsetTableEntry) + sizeof(abi::Header);
  if (implied_length > length) {
    return ErrorImpliedLengthDecoding(implied_length, length);
  }

  Decoder d(base);
  d.length_ = length;
  const auto* const ote = reinterpret_cast<const abi::OffsetTableEntry*>(
      reinterpret_cast<const char*>(base) + sizeof(abi::Header));
  TRY_ASSIGN(d.vm_, ViewMapper::Build(ote, otes));
  return d;
}

}  // namespace capsule
