#include "capsule/abi.h"
#include "capsule/framing.h"
#include "core/crc32c.h"

using core::CRC32C;

namespace capsule {
namespace {

// Gonna be a lot of casting in this file.
template <typename T, typename F>
T* To(F* f) {
  return reinterpret_cast<T*>(f);
}

Result ValidateAlignment(const void* base) {
  const auto uptr = reinterpret_cast<uintptr_t>(base);
  if (0 == (uptr % 8)) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage address 0x{:016x} is not 8B-aligned.", uptr));
}

Result ValidateMinLength(size_t n) {
  constexpr size_t kMinLength = sizeof(abi::FrameHeader) + sizeof(abi::Header) +
                                sizeof(abi::OffsetTableEntry) +
                                sizeof(abi::ChecksummedFrameFooter);
  if (n >= kMinLength) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage length [{}] is less than minimum [{}].", n,
                      kMinLength));
}

Result ValidateLengthMultiple(size_t n) {
  if (0 == (n % 8)) return Result::Ok();
  return Result(
      Code::kInvalidArgument,
      strings::Format("Storage length [{}] is not a multiple of 8.", n));
}

Result ValidateFraming(const abi::FrameHeader* fh, const abi::Header* ih,
                       const abi::ChecksummedFrameFooter* cff, size_t n) {
  // This must be a frame type we comprehend, which is kChecksummed only.
  if (static_cast<abi::FrameType>(fh->frame_type) !=
      abi::FrameType::kChecksummed) {
    return Result(
        Code::kStreamFatal,
        strings::Format("Encoded frame type is [{}] which is not decodeable.",
                        fh->frame_type));
  }

  // The frame's stated length must exactly match the framing.
  if (fh->frame_length != n) {
    return Result(
        Code::kStreamFatal,
        strings::Format(
            "Encoded frame length [{}] differs from memory length [{}].",
            fh->frame_length, n));
  }

  // It is now OK to look at the footer.

  // Reiteration of the type must match.
  if (fh->frame_type != cff->reiterated_frame_type) {
    return Result(Code::kStreamFatal,
                  strings::Format("Encoded frame type in header [{}] not equal "
                                  "to footer frame type [{}].",
                                  fh->frame_type, cff->reiterated_frame_type));
  }

  // The frame's reiterated length must exactly match the header length.
  if (fh->frame_length != cff->reiterated_frame_length) {
    return Result(
        Code::kStreamFatal,
        strings::Format("Header-encoded frame length [{}] differs from "
                        "footer-encoded frame length [{}].",
                        fh->frame_length, cff->reiterated_frame_length));
  }

  // Capsule length and frame length must agree.
  const auto recovered_frame_length = ih->capsule_length +
                                      sizeof(abi::FrameHeader) +
                                      sizeof(abi::ChecksummedFrameFooter);
  if (fh->frame_length != recovered_frame_length) {
    return Result(
        Code::kStreamFatal,
        strings::Format(
            "Encoded frame length [{}] inconsistent with inner "
            "capsule length [{}] which implies a frame of length [{}].",
            fh->frame_length, ih->capsule_length, recovered_frame_length));
  }

  // Every offset table entry requires at least 8B: 4B for the hash, 4B for the
  // value.
  const size_t max_offset_table_entries = (ih->capsule_length) / 8;
  if (ih->offset_table_count > max_offset_table_entries) {
    return Result(
        Code::kCapsuleFatal,
        strings::Format("Capsule encodes offset table count [{}] in excess of "
                        "framing maximum [{}].",
                        ih->offset_table_count, max_offset_table_entries));
  }

  if (ih->offset_table_count == 0) {
    return Result(Code::kCapsuleFatal, "Capsule encodes empty offset table.");
  }

  return Result::Ok();
}

const abi::ChecksummedFrameFooter* BaseToFooter(const void* base, size_t n) {
  return To<const abi::ChecksummedFrameFooter>(
      To<const char>(base) + ((n - sizeof(abi::ChecksummedFrameFooter))));
}

abi::ChecksummedFrameFooter* BaseToFooter(void* base, size_t n) {
  return To<abi::ChecksummedFrameFooter>(
      To<char>(base) + ((n - sizeof(abi::ChecksummedFrameFooter))));
}

CRC32C ComputeCrc(const void* base, size_t n) {
  static_assert((sizeof(abi::ChecksummedFrameFooter) -
                 offsetof(abi::ChecksummedFrameFooter, frame_crc)) == 4);
  return core::ComputeCRC32C(base, n - 4);
}

Result ValidateCrc(const void* base, size_t n) {
  const auto computed = ComputeCrc(base, n);
  const auto stored = BaseToFooter(base, n)->frame_crc;
  if (computed == stored) return Result::Ok();
  return Result(
      Code::kCapsuleFatal,
      strings::Format(
          "Capsule encodes CRC32C of {:08x} but computed CRC32C is {:08x}",
          stored.value(), computed.value()));
}

}  // namespace

// static
Result Framing::Validate(const void* base, size_t n) {
  TRY(ValidateAlignment(base));
  TRY(ValidateMinLength(n));
  TRY(ValidateLengthMultiple(n));
  const auto* const fh = To<const abi::FrameHeader>(base);
  const auto* const ih = To<const abi::Header>(fh + 1);
  static_assert(sizeof(abi::FrameHeader) == sizeof(abi::Header),
                "Dirty trick above only works with equal size structs.");
  const auto* const cff = BaseToFooter(base, n);
  TRY(ValidateFraming(fh, ih, cff, n));
  TRY(ValidateCrc(base, n));
  return Result::Ok();
}

// static
Result Framing::Sign(void* base, size_t n) {
  TRY(ValidateAlignment(base));
  TRY(ValidateMinLength(n));
  auto* const header = To<abi::FrameHeader>(base);
  auto* const footer = BaseToFooter(base, n);
  footer->reiterated_frame_length = header->frame_length;
  footer->reiterated_frame_type = header->frame_type;
  footer->frame_crc = ComputeCrc(base, n);
  return Result::Ok();
}

// static
ResultOr<Framing::UnframedCapsule> Framing::Unframe(
    Storage* s, std::shared_ptr<StorageFactory> fac) {
  TRY(Framing::Validate(s->base(), s->n()));
  const auto* const fh = To<const abi::FrameHeader>(s->base());
  UnframedCapsule c;
  c.enclosed_type = fh->capsule_id_hash;
  TRY_ASSIGN(c.storage, s->Carve(sizeof(abi::FrameHeader),
                                 sizeof(abi::ChecksummedFrameFooter)));
  return std::move(c);
}

// static
ResultOr<Framing::FramedCapsule> Framing::AllocFrame(
    size_t capsule_size, std::shared_ptr<StorageFactory> fac) {
  TRY(ValidateLengthMultiple(capsule_size));
  const auto allocation_size = capsule_size + sizeof(abi::FrameHeader) +
                               sizeof(abi::ChecksummedFrameFooter);
  TRY(ValidateMinLength(allocation_size));

  FramedCapsule fc;
  TRY_ASSIGN(fc.frame_storage,
             Storage::Allocate(std::move(fac), allocation_size));
  TRY_ASSIGN(fc.capsule_storage,
             fc.frame_storage->Carve(sizeof(abi::FrameHeader),
                                     sizeof(abi::ChecksummedFrameFooter)));
  return fc;
}

// static
Result Framing::CompleteFraming(core::CRC32C enclosed_type, FramedCapsule* fc) {
  auto* const fh = fc->frame_storage->DataAsPtrTo<abi::FrameHeader>();
  fh->capsule_id_hash = enclosed_type;
  fh->frame_length = fc->frame_storage->n();
  fh->frame_type = static_cast<uint32_t>(abi::FrameType::kChecksummed);
  return Framing::Sign(fc->frame_storage->base(), fc->frame_storage->n());
}

}  // namespace capsule
