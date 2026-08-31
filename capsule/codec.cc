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

Result ValidateHeader(const abi::Header* h, size_t n) {
  // A capsule must state and reiterate its full length.
  if (h->capsule_length_reiteration != h->capsule_length) {
    return Result(
        Code::kInvalidArgument,  // becomes stream-fatal
        strings::Format("Encoded capsule length [{}] differs from reiterated "
                        "capsule length [{}].",
                        h->capsule_length, h->capsule_length_reiteration));
  }

  // The capsule's stated length must exactly match the framing.
  if (h->capsule_length != n) {
    return Result(
        Code::kInvalidArgument,  // becomes stream-fatal
        strings::Format(
            "Encoded capsule length [{}] differs from framed length [{}].",
            h->capsule_length, n));
  }

  // Every offset table entry requires at least 12B, 4B for the hash, 4B for the
  // pointer, and min 4B for the value itself.
  const size_t max_offset_table_entries =
      (n - sizeof(abi::Header) - sizeof(CRC32C)) / 12;
  if (h->offset_table_count > max_offset_table_entries) {
    return Result(
        Code::kInvalidArgument,  // becomes capsule-fatal
        strings::Format("Capsule encodes offset table count [{}] in excess of "
                        "framing maximum [{}].",
                        h->offset_table_count, max_offset_table_entries));
  }

  if (h->offset_table_count == 0) {
    return Result(Code::kInvalidArgument,  // becomes capsule-fatal
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

abi::OffsetTableEntry* BaseToOffsetTableEntry(void* base, size_t i) {
  return To<abi::OffsetTableEntry>(To<char>(base) + sizeof(abi::Header)) + i;
}

Result ValidateCrc(void* base, size_t n) {
  const auto computed = ComputeCrc(base, n);
  const auto stored = *BaseToCrcPointer(base, n);
  if (computed == stored) return Result::Ok();
  return Result(
      Code::kInvalidArgument,  // becomes capsule-fatal
      strings::Format(
          "Capsule encodes CRC32C of {:08x} but computed CRC32C is {:08x}",
          stored.value(), computed.value()));
}

Result ValidateOffsetTable(void* base, size_t n) {
  const auto* const h = To<abi::Header>(base);
  const auto count = h->offset_table_count;

  // Inclusive bound.
  const uint32_t first_data_dword =
      sizeof(abi::Header) + count * sizeof(abi::OffsetTableEntry);

  // Exclusive bound.
  const uint32_t upper = ((n / 4) - 1) * 4;

  // Offset table may point multiple fields to the same data (to dedup), so we
  // do not check for overlap.
  for (int i = 0; i < count; ++i) {
    const auto* const entry = BaseToOffsetTableEntry(base, i);
    const auto o = entry->data_offset;
    if ((o % 4) != 0) {
      return Result(Code::kInvalidArgument,  // capsule fatal
                    strings::Format(
                        "Capsule object table for type 0x{:08x} encodes bogus "
                        "intra-capsule offset [{}]; not 4B-aligned. ",
                        entry->field_hash.value(), o));
    }
    if (o < first_data_dword || o >= upper) {
      return Result(
          Code::kInvalidArgument,  // capsule fatal
          strings::Format(
              "Capsule object table for type 0x{:08x} encodes bogus "
              "intra-capsule offset [{}] on capsule of size [{}]. "
              "Expected range [{}, {}).",
              entry->field_hash.value(), o, n, first_data_dword, upper));
    }
  }
  return Result::Ok();
}

}  // namespace

// static
Result Codec::Validate(void* base, size_t n) {
  // TODO give useful error codes here.
  TRY(ValidateAlignment(base));
  TRY(ValidateMinLength(n));
  TRY(ValidateLengthMultiple(n));
  TRY(ValidateHeader(To<abi::Header>(base), n));
  TRY(ValidateCrc(base, n));
  TRY(ValidateOffsetTable(base, n));
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
