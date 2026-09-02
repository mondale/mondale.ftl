#include "capsule/abi.h"
#include "capsule/codec.h"
#include "core/crc32c.h"

using core::CRC32C;

namespace capsule {
namespace {

// Gonna be a lot of casting in this file.
template <typename T, typename F>
T* To(F* f) {
  return reinterpret_cast<T*>(f);
}

Result ValidateAlignment(void* base) {
  const auto uptr = reinterpret_cast<uintptr_t>(base);
  if (0 == (uptr % 8)) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage address 0x{:016x} is not 8B-aligned.", uptr));
}

Result ValidateMinLength(size_t n) {
  constexpr size_t kMinLength = 4 +  // capsule ID hash
                                4 +  // total capsule length
                                4 +  // offset table count
                                4 +  // capsule length repeated (end of header)
                                8 +  // at least one offset table entry
                                4;   // CRC at the end.
  if (n >= kMinLength) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage length [{}] is less than minimum [{}].", n,
                      kMinLength));
}

Result ValidateLengthMultiple(size_t n) {
  if (0 == (n % 4)) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage length [{}] is not a multiple of 4.", n));
}

Result ValidateFrameHeader(const abi::FrameHeader* fh, const abi::Header* ih,
                           size_t n) {
  // Capsule length and frame length must agree.
  const auto recovered_frame_length =
      ih->capsule_length + sizeof(abi::FrameHeader) + sizeof(core::CRC32C);
  if (fh->frame_length != recovered_frame_length) {
    return Result(
        Code::kInvalidArgument,  // TODO becomes stream-fatal
        strings::Format(
            "Encoded frame length [{}] inconsistent with inner "
            "capsule length [{}] which implies a frame of length [{}].",
            fh->frame_length, ih->capsule_length, recovered_frame_length));
  }

  // The frame's stated length must exactly match the framing.
  if (fh->frame_length != n) {
    return Result(
        Code::kInvalidArgument,  // TODO becomes stream-fatal
        strings::Format(
            "Encoded frame length [{}] differs from memory length [{}].",
            fh->frame_length, n));
  }

  // Every offset table entry requires at least 8B: 4B for the hash, 4B for the
  // value.
  const size_t max_offset_table_entries = (ih->capsule_length) / 8;
  if (ih->offset_table_count > max_offset_table_entries) {
    return Result(
        Code::kInvalidArgument,  // TODO becomes capsule-fatal
        strings::Format("Capsule encodes offset table count [{}] in excess of "
                        "framing maximum [{}].",
                        ih->offset_table_count, max_offset_table_entries));
  }

  if (ih->offset_table_count == 0) {
    return Result(Code::kInvalidArgument,  // TODO becomes capsule-fatal
                  "Capsule encodes empty offset table.");
  }

  return Result::Ok();
}

CRC32C* BaseToCrcPointer(void* base, size_t n) {
  // Return the last DWORD.
  return To<CRC32C>(To<uint32_t>(base) + ((n / 4) - 1));
}

CRC32C ComputeCrc(void* base, size_t n) {
  return core::ComputeCRC32C(base, n - 4);
}

Result ValidateCrc(void* base, size_t n) {
  const auto computed = ComputeCrc(base, n);
  const auto stored = *BaseToCrcPointer(base, n);
  if (computed == stored) return Result::Ok();
  return Result(
      Code::kInvalidArgument,  // TODO becomes capsule-fatal
      strings::Format(
          "Capsule encodes CRC32C of {:08x} but computed CRC32C is {:08x}",
          stored.value(), computed.value()));
}

}  // namespace

// static
Result Codec::Validate(void* base, size_t n) {
  // TODO give useful error codes here.
  TRY(ValidateAlignment(base));
  TRY(ValidateMinLength(n));
  TRY(ValidateLengthMultiple(n));
  auto* const fh = To<abi::FrameHeader>(base);
  auto* const ih = To<abi::Header>(fh + 1);
  static_assert(sizeof(abi::FrameHeader) == sizeof(abi::Header),
                "Dirty trick above only works with equal size structs.");
  TRY(ValidateFrameHeader(fh, ih, n));
  TRY(ValidateCrc(base, n));
  return Result::Ok();
}

// static
Result Codec::Sign(void* base, size_t n) {
  TRY(ValidateAlignment(base));
  TRY(ValidateMinLength(n));
  const auto computed = ComputeCrc(base, n);
  *BaseToCrcPointer(base, n) = computed;
  return Result::Ok();
}

}  // namespace capsule
