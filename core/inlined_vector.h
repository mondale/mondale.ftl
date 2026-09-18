#ifndef CORE_INLINED_VECTOR_H_
#define CORE_INLINED_VECTOR_H_

#include <cstddef>
#include <new>
#include <utility>

namespace core {

template <typename T, size_t N>
class InlinedVector {
 public:
  static_assert(N > 0, "InlinedVector capacity N must be greater than 0");

  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = T*;
  using const_iterator = const T*;

  InlinedVector() noexcept
      : size_(0), capacity_(N), data_(GetInlineStorage()) {}

  ~InlinedVector() {
    clear();
    DeallocateIfHeap();
  }

  InlinedVector(const InlinedVector& other)
      : size_(0), capacity_(N), data_(GetInlineStorage()) {
    reserve(other.size_);
    for (size_t i = 0; i < other.size_; ++i) {
      push_back(other[i]);
    }
  }

  InlinedVector(InlinedVector&& other) noexcept {
    if (other.IsInline()) {
      size_ = 0;
      capacity_ = N;
      data_ = GetInlineStorage();
      reserve(other.size_);
      for (size_t i = 0; i < other.size_; ++i) {
        push_back(std::move(other[i]));
      }
      other.clear();
    } else {
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = other.data_;
      other.size_ = 0;
      other.capacity_ = N;
      other.data_ = other.GetInlineStorage();
    }
  }

  InlinedVector& operator=(const InlinedVector& other) {
    if (this != &other) {
      clear();
      reserve(other.size_);
      for (size_t i = 0; i < other.size_; ++i) {
        push_back(other[i]);
      }
    }
    return *this;
  }

  InlinedVector& operator=(InlinedVector&& other) noexcept {
    if (this != &other) {
      clear();
      DeallocateIfHeap();
      if (other.IsInline()) {
        size_ = 0;
        capacity_ = N;
        data_ = GetInlineStorage();
        reserve(other.size_);
        for (size_t i = 0; i < other.size_; ++i) {
          push_back(std::move(other[i]));
        }
        other.clear();
      } else {
        size_ = other.size_;
        capacity_ = other.capacity_;
        data_ = other.data_;
        other.size_ = 0;
        other.capacity_ = N;
        other.data_ = other.GetInlineStorage();
      }
    }
    return *this;
  }

  void push_back(const T& value) { EmplaceBackInternal(value); }

  void push_back(T&& value) { EmplaceBackInternal(std::move(value)); }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    if (size_ == capacity_) {
      Grow();
    }
    T* addr = data_ + size_;
    ::new (static_cast<void*>(addr)) T(std::forward<Args>(args)...);
    ++size_;
    return *addr;
  }

  void pop_back() {
    if (size_ > 0) {
      --size_;
      data_[size_].~T();
    }
  }

  void clear() noexcept {
    for (size_t i = 0; i < size_; ++i) {
      data_[i].~T();
    }
    size_ = 0;
  }

  void reserve(size_t new_capacity) {
    if (new_capacity <= capacity_) {
      return;
    }
    Reallocate(new_capacity);
  }

  size_t size() const noexcept { return size_; }
  size_t capacity() const noexcept { return capacity_; }
  bool empty() const noexcept { return size_ == 0; }

  pointer data() noexcept { return data_; }
  const_pointer data() const noexcept { return data_; }

  reference operator[](size_t index) { return data_[index]; }
  const_reference operator[](size_t index) const { return data_[index]; }

  iterator begin() noexcept { return data_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator cbegin() const noexcept { return data_; }

  iterator end() noexcept { return data_ + size_; }
  const_iterator end() const noexcept { return data_ + size_; }
  const_iterator cend() const noexcept { return data_ + size_; }

 private:
  pointer GetInlineStorage() noexcept {
    return reinterpret_cast<pointer>(inline_storage_);
  }

  const_pointer GetInlineStorage() const noexcept {
    return reinterpret_cast<const_pointer>(inline_storage_);
  }

  bool IsInline() const noexcept { return data_ == GetInlineStorage(); }

  void DeallocateIfHeap() noexcept {
    if (!IsInline()) {
      ::operator delete[](static_cast<void*>(data_));
    }
  }

  template <typename U>
  void EmplaceBackInternal(U&& value) {
    if (size_ == capacity_) {
      Grow();
    }
    T* addr = data_ + size_;
    ::new (static_cast<void*>(addr)) T(std::forward<U>(value));
    ++size_;
  }

  void Grow() {
    size_t new_capacity = capacity_ * 2;
    Reallocate(new_capacity);
  }

  void Reallocate(size_t new_capacity) {
    pointer new_data =
        static_cast<pointer>(::operator new[](new_capacity * sizeof(T)));
    for (size_t i = 0; i < size_; ++i) {
      ::new (static_cast<void*>(new_data + i)) T(std::move(data_[i]));
      data_[i].~T();
    }
    DeallocateIfHeap();
    data_ = new_data;
    capacity_ = new_capacity;
  }

  size_t size_;
  size_t capacity_;
  pointer data_;
  alignas(T) char inline_storage_[sizeof(T) * N];
};

}  // namespace core

#endif  // #ifndef CORE_INLINED_VECTOR_H_
