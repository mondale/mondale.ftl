#ifndef CORE_SMALL_MAP_H_
#define CORE_SMALL_MAP_H_

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <utility>

#include "core/inlined_vector.h"

namespace core {

template <typename Key, typename T, size_t N = 2,
          typename Compare = std::less<Key>>
class SmallMap {
 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

 private:
  using UnderlyingVector = InlinedVector<value_type, N>;

  struct KeyEqual {
    bool operator()(const Key& lhs, const Key& rhs) const {
      Compare comp;
      return !comp(lhs, rhs) && !comp(rhs, lhs);
    }
  };

 public:
  class iterator {
   public:
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename SmallMap::value_type;
    using difference_type = typename SmallMap::difference_type;
    using pointer = typename SmallMap::pointer;
    using reference = typename SmallMap::reference;

    iterator() noexcept : ptr_(nullptr) {}
    explicit iterator(pointer ptr) noexcept : ptr_(ptr) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    iterator& operator++() noexcept {
      ++ptr_;
      return *this;
    }

    iterator operator++(int) noexcept {
      iterator tmp = *this;
      ++ptr_;
      return tmp;
    }

    iterator& operator--() noexcept {
      --ptr_;
      return *this;
    }

    iterator operator--(int) noexcept {
      iterator tmp = *this;
      --ptr_;
      return tmp;
    }

    iterator& operator+=(difference_type n) noexcept {
      ptr_ += n;
      return *this;
    }

    iterator& operator-=(difference_type n) noexcept {
      ptr_ -= n;
      return *this;
    }

    friend iterator operator+(iterator it, difference_type n) noexcept {
      return iterator(it.ptr_ + n);
    }

    friend iterator operator+(difference_type n, iterator it) noexcept {
      return iterator(it.ptr_ + n);
    }

    friend iterator operator-(iterator it, difference_type n) noexcept {
      return iterator(it.ptr_ - n);
    }

    friend difference_type operator-(iterator a, iterator b) noexcept {
      return a.ptr_ - b.ptr_;
    }

    reference operator[](difference_type n) const { return ptr_[n]; }

    friend bool operator==(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ == b.ptr_;
    }

    friend bool operator!=(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ != b.ptr_;
    }

    friend bool operator<(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ < b.ptr_;
    }

    friend bool operator<=(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ <= b.ptr_;
    }

    friend bool operator>(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ > b.ptr_;
    }

    friend bool operator>=(const iterator& a, const iterator& b) noexcept {
      return a.ptr_ >= b.ptr_;
    }

   private:
    pointer ptr_;
    friend class SmallMap;
  };

  class const_iterator {
   public:
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename SmallMap::value_type;
    using difference_type = typename SmallMap::difference_type;
    using pointer = typename SmallMap::const_pointer;
    using reference = typename SmallMap::const_reference;

    const_iterator() noexcept : ptr_(nullptr) {}
    explicit const_iterator(pointer ptr) noexcept : ptr_(ptr) {}
    const_iterator(iterator it) noexcept : ptr_(it.ptr_) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    const_iterator& operator++() noexcept {
      ++ptr_;
      return *this;
    }

    const_iterator operator++(int) noexcept {
      const_iterator tmp = *this;
      ++ptr_;
      return tmp;
    }

    const_iterator& operator--() noexcept {
      --ptr_;
      return *this;
    }

    const_iterator operator--(int) noexcept {
      const_iterator tmp = *this;
      --ptr_;
      return tmp;
    }

    const_iterator& operator+=(difference_type n) noexcept {
      ptr_ += n;
      return *this;
    }

    const_iterator& operator-=(difference_type n) noexcept {
      ptr_ -= n;
      return *this;
    }

    friend const_iterator operator+(const_iterator it,
                                    difference_type n) noexcept {
      return const_iterator(it.ptr_ + n);
    }

    friend const_iterator operator+(difference_type n,
                                    const_iterator it) noexcept {
      return const_iterator(it.ptr_ + n);
    }

    friend const_iterator operator-(const_iterator it,
                                    difference_type n) noexcept {
      return const_iterator(it.ptr_ - n);
    }

    friend difference_type operator-(const_iterator a,
                                     const_iterator b) noexcept {
      return a.ptr_ - b.ptr_;
    }

    reference operator[](difference_type n) const { return ptr_[n]; }

    friend bool operator==(const const_iterator& a,
                           const const_iterator& b) noexcept {
      return a.ptr_ == b.ptr_;
    }

    friend bool operator!=(const const_iterator& a,
                           const const_iterator& b) noexcept {
      return a.ptr_ != b.ptr_;
    }

    friend bool operator<(const const_iterator& a,
                          const const_iterator& b) noexcept {
      return a.ptr_ < b.ptr_;
    }

    friend bool operator<=(const const_iterator& a, const_iterator b) noexcept {
      return a.ptr_ <= b.ptr_;
    }

    friend bool operator>(const const_iterator& a,
                          const const_iterator& b) noexcept {
      return a.ptr_ > b.ptr_;
    }

    friend bool operator>=(const const_iterator& a,
                           const const_iterator& b) noexcept {
      return a.ptr_ >= b.ptr_;
    }

   private:
    pointer ptr_;
    friend class SmallMap;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  SmallMap() = default;
  ~SmallMap() = default;

  SmallMap(const SmallMap&) = default;
  SmallMap(SmallMap&&) noexcept = default;

  SmallMap& operator=(const SmallMap&) = default;
  SmallMap& operator=(SmallMap&&) noexcept = default;

  SmallMap(std::initializer_list<value_type> init,
           const Compare& comp = Compare())
      : comp_(comp) {
    for (auto& item : init) {
      insert(item);
    }
  }

  iterator begin() noexcept { return iterator(elems_.data()); }
  const_iterator begin() const noexcept {
    return const_iterator(elems_.data());
  }
  const_iterator cbegin() const noexcept {
    return const_iterator(elems_.data());
  }

  iterator end() noexcept { return iterator(elems_.data() + elems_.size()); }
  const_iterator end() const noexcept {
    return const_iterator(elems_.data() + elems_.size());
  }
  const_iterator cend() const noexcept {
    return const_iterator(elems_.data() + elems_.size());
  }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  bool empty() const noexcept { return elems_.empty(); }
  size_t size() const noexcept { return elems_.size(); }
  size_t capacity() const noexcept { return elems_.capacity(); }

  void clear() noexcept { elems_.clear(); }

  std::pair<iterator, bool> insert(const value_type& value) {
    return emplace_impl(value.first, value.second);
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    return emplace_impl(value.first, std::move(value.second));
  }

  template <typename P>
  std::pair<iterator, bool> insert(P&& value) {
    return emplace_impl(value.first,
                        std::forward<typename P::second_type>(value.second));
  }

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
  }

  void insert(std::initializer_list<value_type> ilist) {
    for (auto& item : ilist) {
      insert(item);
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    value_type val(std::forward<Args>(args)...);
    return insert(std::move(val));
  }

  template <typename K, typename... Args>
  std::pair<iterator, bool> try_emplace(const K& k, Args&&... args) {
    auto it = find_internal(k);
    if (it != end()) {
      return {it, false};
    }
    elems_.emplace_back(std::piecewise_construct, std::forward_as_tuple(k),
                        std::forward_as_tuple(std::forward<Args>(args)...));
    return {end() - 1, true};
  }

  template <typename K, typename... Args>
  std::pair<iterator, bool> try_emplace(K&& k, Args&&... args) {
    auto it = find_internal(k);
    if (it != end()) {
      return {it, false};
    }
    elems_.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(std::forward<K>(k)),
                        std::forward_as_tuple(std::forward<Args>(args)...));
    return {end() - 1, true};
  }

  mapped_type& operator[](const Key& key) {
    auto it = find_internal(key);
    if (it != end()) {
      return it->second;
    }
    elems_.emplace_back(std::piecewise_construct, std::forward_as_tuple(key),
                        std::forward_as_tuple());
    return (end() - 1)->second;
  }

  mapped_type& operator[](Key&& key) {
    auto it = find_internal(key);
    if (it != end()) {
      return it->second;
    }
    elems_.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(std::move(key)),
                        std::forward_as_tuple());
    return (end() - 1)->second;
  }

  mapped_type& at(const Key& key) {
    auto it = find_internal(key);
    if (it == end()) {
      throw std::out_of_range("SmallMap::at: key not found");
    }
    return it->second;
  }

  const mapped_type& at(const Key& key) const {
    auto it = find_internal(key);
    if (it == end()) {
      throw std::out_of_range("SmallMap::at: key not found");
    }
    return it->second;
  }

  iterator erase(const_iterator pos) {
    size_t idx = pos - cbegin();
    // Destroy the element being erased
    elems_[idx].~value_type();

    // Shift subsequent elements left using move-construction
    for (size_t i = idx + 1; i < elems_.size(); ++i) {
      ::new (static_cast<void*>(&elems_[i - 1]))
          value_type(std::move(elems_[i]));
    }

    // Pop the final slot (whose object was moved from)
    elems_.pop_back();
    return begin() + idx;
  }

  iterator erase(iterator pos) { return erase(const_iterator(pos)); }

  size_t erase(const Key& key) {
    auto it = find_internal(key);
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t start_idx = first - cbegin();
    size_t count = last - first;
    if (count == 0) {
      return begin() + start_idx;
    }
    size_t end_idx = start_idx + count;

    // Destroy elements in the erased range
    for (size_t i = start_idx; i < end_idx; ++i) {
      elems_[i].~value_type();
    }

    // Shift remaining elements left
    for (size_t i = end_idx; i < elems_.size(); ++i) {
      ::new (static_cast<void*>(&elems_[i - count]))
          value_type(std::move(elems_[i]));
    }

    // Pop back for each removed slot
    for (size_t i = 0; i < count; ++i) {
      elems_.pop_back();
    }

    return begin() + start_idx;
  }

  iterator find(const Key& key) { return find_internal(key); }

  const_iterator find(const Key& key) const { return find_internal(key); }

  bool contains(const Key& key) const { return find_internal(key) != end(); }

  size_t count(const Key& key) const {
    return find_internal(key) != end() ? 1 : 0;
  }

  std::pair<iterator, iterator> equal_range(const Key& key) {
    auto it = find_internal(key);
    if (it == end()) {
      return {end(), end()};
    }
    return {it, it + 1};
  }

  std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
    auto it = find_internal(key);
    if (it == end()) {
      return {end(), end()};
    }
    return {it, it + 1};
  }

  iterator lower_bound(const Key& key) {
    Compare comp;
    for (auto it = begin(); it != end(); ++it) {
      if (!comp(it->first, key)) {
        return it;
      }
    }
    return end();
  }

  const_iterator lower_bound(const Key& key) const {
    KeyEqual eq;
    Compare comp;
    for (auto it = begin(); it != end(); ++it) {
      if (!comp(it->first, key)) {
        return it;
      }
    }
    return end();
  }

  iterator upper_bound(const Key& key) {
    Compare comp;
    for (auto it = begin(); it != end(); ++it) {
      if (comp(key, it->first)) {
        return it;
      }
    }
    return end();
  }

  const_iterator upper_bound(const Key& key) const {
    Compare comp;
    for (auto it = begin(); it != end(); ++it) {
      if (comp(key, it->first)) {
        return it;
      }
    }
    return end();
  }

  key_compare key_comp() const { return comp_; }

 private:
  iterator find_internal(const Key& key) {
    KeyEqual eq;
    for (auto it = begin(); it != end(); ++it) {
      if (eq(it->first, key)) {
        return it;
      }
    }
    return end();
  }

  const_iterator find_internal(const Key& key) const {
    KeyEqual eq;
    for (auto it = begin(); it != end(); ++it) {
      if (eq(it->first, key)) {
        return it;
      }
    }
    return end();
  }

  template <typename K, typename M>
  std::pair<iterator, bool> emplace_impl(K&& key, M&& obj) {
    auto it = find_internal(key);
    if (it != end()) {
      return {it, false};
    }
    elems_.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(std::forward<K>(key)),
                        std::forward_as_tuple(std::forward<M>(obj)));
    return {end() - 1, true};
  }

  UnderlyingVector elems_;
  [[no_unique_address]] Compare comp_;
};

}  // namespace core

#endif  // CORE_SMALL_MAP_H_
