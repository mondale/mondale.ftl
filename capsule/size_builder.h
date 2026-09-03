#ifndef CAPSULE_SIZE_BUILDER_H_
#define CAPSULE_SIZE_BUILDER_H_

#include <concepts>
#include <string>
#include <type_traits>
#include <vector>

#include "capsule/abi.h"

namespace capsule {
namespace internal {

template <typename T>
concept HasComputeStorageSize = requires(T t) { t.ComputeStorageSize(); };

template <typename T>
concept HasVecComputeStorageSize = requires(T t) { t[0].ComputeStorageSize(); };

template <typename T>
concept PrimitiveVectors = std::is_same_v<T, std::vector<int8_t>> ||
                           std::is_same_v<T, std::vector<uint8_t>> ||
                           std::is_same_v<T, std::vector<int16_t>> ||
                           std::is_same_v<T, std::vector<uint16_t>> ||
                           std::is_same_v<T, std::vector<int32_t>> ||
                           std::is_same_v<T, std::vector<uint32_t>> ||
                           std::is_same_v<T, std::vector<int64_t>> ||
                           std::is_same_v<T, std::vector<uint64_t>> ||
                           std::is_same_v<T, std::vector<float>> ||
                           std::is_same_v<T, std::vector<double>>;

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
    // 4B for length plus all the bytes in the string.
    AddVariableLengthField(s.length() + 4);
  }

  template <internal::PrimitiveVectors T>
  void Add(const T& v) {
    // 4B for number of elements, plus all the bytes in data.
    const size_t element_size = sizeof(typename T::value_type);
    AddVariableLengthField(4 + element_size * v.size());
  }

  template <>
  void Add<std::vector<std::string>>(const std::vector<std::string>& v) {
    // Every string has an individual byte length, and each string's encoding
    // is rounded up to a multiple of 8.
    size_t bytes = sizeof(abi::VectorHeader);
    for (const auto& s : v) {
      bytes += (4 + s.length() + 7) / 8 * 8;
    }
    AddVariableLengthField(bytes);
  }

  // Handler for a scalar capsule.
  template <typename T>
    requires internal::HasComputeStorageSize<T>
  void Add(const T& t) {
    Add32bField();  // ote
    payload_bytes_ += t.ComputeStorageSize();
  }

  // Handler for a vector of capsules.
  template <typename T>
    requires internal::HasVecComputeStorageSize<T>
  void Add(const T& t) {
    Add32bField();  // ote
    payload_bytes_ += sizeof(abi::VectorHeader);
    for (const auto& elem : t) {
      payload_bytes_ += elem.ComputeStorageSize();
    }
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
