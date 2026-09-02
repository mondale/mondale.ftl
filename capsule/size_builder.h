#ifndef CAPSULE_SIZE_BUILDER_H_
#define CAPSULE_SIZE_BUILDER_H_

#include <concepts>
#include <string>
#include <vector>

#include "capsule/abi.h"

namespace capsule {
namespace internal {

template <typename T>
concept HasComputeStorageSize = requires(T t) { t.ComputeStorageSize(); };

}  // namespace internal

class SizeBuilder final {
 public:
  SizeBuilder() = default;

  size_t Build() const {
    return payload_bytes_ +  // vars 8B and higher
           ote_count_ *
               sizeof(abi::OffsetTableEntry) +  // small vars & pointers
           sizeof(abi::Header);                 // header
  }

  // If this is called, you need to add a template specialization below.
  template <typename T>
  void Add(const T& t) = delete;

  template <>
  void Add<bool>(const bool&) {
    Add8bField();
  }

  template <>
  void Add<uint8_t>(const uint8_t&) {
    Add8bField();
  }

  template <>
  void Add<int8_t>(const int8_t&) {
    Add8bField();
  }

  template <>
  void Add<uint16_t>(const uint16_t&) {
    Add16bField();
  }

  template <>
  void Add<int16_t>(const int16_t&) {
    Add16bField();
  }

  template <>
  void Add<int32_t>(const int32_t&) {
    Add32bField();
  }

  template <>
  void Add<uint32_t>(const uint32_t&) {
    Add32bField();
  }

  template <>
  void Add<float>(const float&) {
    Add32bField();
  }

  template <>
  void Add<int64_t>(const int64_t&) {
    Add64bField();
  }

  template <>
  void Add<uint64_t>(const uint64_t&) {
    Add64bField();
  }

  template <>
  void Add<double>(const double&) {
    Add64bField();
  }

  template <>
  void Add<std::string>(const std::string& s) {
    AddVariableLengthField(s.length() + 4);
  }

  template <typename T>
  void Add(const std::vector<T>& v) {
    payload_bytes_ += sizeof(abi::VectorHeader);
    for (const auto& item : v) {
      Add(item);
    }
  }

  template <typename T>
    requires internal::HasComputeStorageSize<T>
  void Add(const T& t) {
    payload_bytes_ += t.ComputeStorageSize();
  }

  SizeBuilder& Add8bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add16bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add32bField() {  // encodes in the OTE
    ote_count_++;
    return *this;
  }

  SizeBuilder& Add64bField() {  // encodes in the payload area
    ote_count_++;
    payload_bytes_ += 8;
    return *this;
  }

  SizeBuilder& AddVariableLengthField(size_t payload) {
    ote_count_++;
    payload_bytes_ += ((payload + 7) / 8 * 8);
    return *this;
  }

 private:
  int ote_count_ = 0;
  size_t payload_bytes_ = 0;
};

}  // namespace capsule

#endif  // #ifndef CAPSULE_SIZE_BUILDER_H_
