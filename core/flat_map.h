#ifndef CORE_FLAT_MAP_H_
#define CORE_FLAT_MAP_H_

#include <algorithm>
#include <compare>
#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

#include "core/vocabulary.h"

namespace core {

template <typename Key, typename Value, typename Compare = std::less<Key>>
class FlatMap {
 public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key, Value>;
  using size_type = typename std::vector<value_type>::size_type;
  using difference_type = typename std::vector<value_type>::difference_type;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::vector<value_type>::pointer;
  using const_pointer = typename std::vector<value_type>::const_pointer;
  using iterator = typename std::vector<value_type>::iterator;
  using const_iterator = typename std::vector<value_type>::const_iterator;
  using reverse_iterator = typename std::vector<value_type>::reverse_iterator;
  using const_reverse_iterator =
      typename std::vector<value_type>::const_reverse_iterator;

  FlatMap() = default;
  explicit FlatMap(const Compare& comp) : comp_(comp) {}

  FlatMap(std::initializer_list<value_type> init,
          const Compare& comp = Compare())
      : comp_(comp) {
    insert(init.begin(), init.end());
  }

  // Iterators
  iterator begin() { return c_.begin(); }
  const_iterator begin() const { return c_.begin(); }
  iterator end() { return c_.end(); }
  const_iterator end() const { return c_.end(); }

  const_iterator cbegin() const { return c_.cbegin(); }
  const_iterator cend() const { return c_.cend(); }

  reverse_iterator rbegin() { return c_.rbegin(); }
  const_reverse_iterator rbegin() const { return c_.rbegin(); }
  reverse_iterator rend() { return c_.rend(); }
  const_reverse_iterator rend() const { return c_.rend(); }

  const_reverse_iterator crbegin() const { return c_.crbegin(); }
  const_reverse_iterator crend() const { return c_.crend(); }

  // Capacity
  bool empty() const { return c_.empty(); }
  size_type size() const { return c_.size(); }
  size_type max_size() const { return c_.max_size(); }
  void reserve(size_type n) { c_.reserve(n); }
  size_type capacity() const { return c_.capacity(); }
  void shrink_to_fit() { c_.shrink_to_fit(); }

  // Modifiers
  void clear() { c_.clear(); }

  std::pair<iterator, bool> insert(const value_type& v);
  std::pair<iterator, bool> insert(value_type&& v);

  template <typename P>
  std::pair<iterator, bool> insert(P&& v);

  template <typename InputIt>
  void insert(InputIt first, InputIt last);

  void insert(std::initializer_list<value_type> ilist);

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args);

  template <typename... Args>
  iterator emplace_hint(const_iterator hint, Args&&... args);

  iterator erase(iterator pos);
  iterator erase(const_iterator pos);
  size_type erase(const Key& k);
  iterator erase(const_iterator first, const_iterator last);

  void swap(FlatMap& other) {
    c_.swap(other.c_);
    std::swap(comp_, other.comp_);
  }

  // Element access
  Value& at(const Key& k);
  const Value& at(const Key& k) const;

  Value& operator[](const Key& k);
  Value& operator[](Key&& k);

  // Lookup
  size_type count(const Key& k) const;
  bool contains(const Key& k) const;

  iterator find(const Key& k);
  const_iterator find(const Key& k) const;

  std::pair<iterator, iterator> equal_range(const Key& k);
  std::pair<const_iterator, const_iterator> equal_range(const Key& k) const;

  iterator lower_bound(const Key& k);
  const_iterator lower_bound(const Key& k) const;

  iterator upper_bound(const Key& k);
  const_iterator upper_bound(const Key& k) const;

  // Observers
  key_compare key_comp() const { return comp_; }

  bool operator==(const FlatMap& other) const { return c_ == other.c_; }

  std::strong_ordering operator<=>(const FlatMap& other) const {
    return c_ <=> other.c_;
  }

 private:
  std::vector<value_type> c_;
  Compare comp_;

  struct KeyCompareWrapper {
    Compare comp;
    bool operator()(const value_type& x, const Key& y) const {
      return comp(x.first, y);
    }
    bool operator()(const Key& x, const value_type& y) const {
      return comp(x, y.first);
    }
    bool operator()(const value_type& x, const value_type& y) const {
      return comp(x.first, y.first);
    }
  };

  iterator FindLowerBound(const Key& k);
  const_iterator FindLowerBound(const Key& k) const;
};

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::const_iterator
FlatMap<Key, Value, Compare>::FindLowerBound(const Key& k) const {
  return std::lower_bound(c_.begin(), c_.end(), k, KeyCompareWrapper{comp_});
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::FindLowerBound(const Key& k) {
  return std::lower_bound(c_.begin(), c_.end(), k, KeyCompareWrapper{comp_});
}

template <typename Key, typename Value, typename Compare>
std::pair<typename FlatMap<Key, Value, Compare>::iterator, bool>
FlatMap<Key, Value, Compare>::insert(const value_type& v) {
  auto it = FindLowerBound(v.first);
  if (it != c_.end() && !comp_(v.first, it->first) &&
      !comp_(it->first, v.first)) {
    return {it, false};
  }
  return {c_.insert(it, v), true};
}

template <typename Key, typename Value, typename Compare>
std::pair<typename FlatMap<Key, Value, Compare>::iterator, bool>
FlatMap<Key, Value, Compare>::insert(value_type&& v) {
  auto it = FindLowerBound(v.first);
  if (it != c_.end() && !comp_(v.first, it->first) &&
      !comp_(it->first, v.first)) {
    return {it, false};
  }
  return {c_.insert(it, std::move(v)), true};
}

template <typename Key, typename Value, typename Compare>
template <typename P>
std::pair<typename FlatMap<Key, Value, Compare>::iterator, bool>
FlatMap<Key, Value, Compare>::insert(P&& v) {
  return emplace(std::forward<P>(v).first, std::forward<P>(v).second);
}

template <typename Key, typename Value, typename Compare>
template <typename InputIt>
void FlatMap<Key, Value, Compare>::insert(InputIt first, InputIt last) {
  for (auto it = first; it != last; ++it) {
    insert(*it);
  }
}

template <typename Key, typename Value, typename Compare>
void FlatMap<Key, Value, Compare>::insert(
    std::initializer_list<value_type> ilist) {
  insert(ilist.begin(), ilist.end());
}

template <typename Key, typename Value, typename Compare>
template <typename... Args>
std::pair<typename FlatMap<Key, Value, Compare>::iterator, bool>
FlatMap<Key, Value, Compare>::emplace(Args&&... args) {
  value_type v(std::forward<Args>(args)...);
  return insert(std::move(v));
}

template <typename Key, typename Value, typename Compare>
template <typename... Args>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::emplace_hint(const_iterator hint,
                                           Args&&... args) {
  value_type v(std::forward<Args>(args)...);
  auto p = insert(std::move(v));
  return p.first;
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::erase(iterator pos) {
  return c_.erase(pos);
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::erase(const_iterator pos) {
  return c_.erase(pos);
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::size_type
FlatMap<Key, Value, Compare>::erase(const Key& k) {
  auto it = find(k);
  if (it == c_.end()) {
    return 0;
  }
  c_.erase(it);
  return 1;
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::erase(const_iterator first, const_iterator last) {
  return c_.erase(first, last);
}

template <typename Key, typename Value, typename Compare>
Value& FlatMap<Key, Value, Compare>::at(const Key& k) {
  auto it = find(k);
  CHECK(it != c_.end());
  return it->second;
}

template <typename Key, typename Value, typename Compare>
const Value& FlatMap<Key, Value, Compare>::at(const Key& k) const {
  auto it = find(k);
  CHECK(it != c_.end());
  return it->second;
}

template <typename Key, typename Value, typename Compare>
Value& FlatMap<Key, Value, Compare>::operator[](const Key& k) {
  auto it = FindLowerBound(k);
  if (it != c_.end() && !comp_(k, it->first) && !comp_(it->first, k)) {
    return it->second;
  }
  return c_.insert(it, value_type(k, Value()))->second;
}

template <typename Key, typename Value, typename Compare>
Value& FlatMap<Key, Value, Compare>::operator[](Key&& k) {
  auto it = FindLowerBound(k);
  if (it != c_.end() && !comp_(k, it->first) && !comp_(it->first, k)) {
    return it->second;
  }
  Key k_copy = k;
  return c_.insert(it, value_type(std::move(k), Value()))->second;
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::size_type
FlatMap<Key, Value, Compare>::count(const Key& k) const {
  return contains(k) ? 1 : 0;
}

template <typename Key, typename Value, typename Compare>
bool FlatMap<Key, Value, Compare>::contains(const Key& k) const {
  auto it = FindLowerBound(k);
  return it != c_.end() && !comp_(k, it->first) && !comp_(it->first, k);
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::find(const Key& k) {
  auto it = FindLowerBound(k);
  if (it != c_.end() && !comp_(k, it->first) && !comp_(it->first, k)) {
    return it;
  }
  return c_.end();
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::const_iterator
FlatMap<Key, Value, Compare>::find(const Key& k) const {
  auto it = FindLowerBound(k);
  if (it != c_.end() && !comp_(k, it->first) && !comp_(it->first, k)) {
    return it;
  }
  return c_.end();
}

template <typename Key, typename Value, typename Compare>
std::pair<typename FlatMap<Key, Value, Compare>::iterator,
          typename FlatMap<Key, Value, Compare>::iterator>
FlatMap<Key, Value, Compare>::equal_range(const Key& k) {
  auto lb = lower_bound(k);
  auto ub = upper_bound(k);
  return {lb, ub};
}

template <typename Key, typename Value, typename Compare>
std::pair<typename FlatMap<Key, Value, Compare>::const_iterator,
          typename FlatMap<Key, Value, Compare>::const_iterator>
FlatMap<Key, Value, Compare>::equal_range(const Key& k) const {
  auto lb = lower_bound(k);
  auto ub = upper_bound(k);
  return {lb, ub};
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::lower_bound(const Key& k) {
  return FindLowerBound(k);
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::const_iterator
FlatMap<Key, Value, Compare>::lower_bound(const Key& k) const {
  return FindLowerBound(k);
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::iterator
FlatMap<Key, Value, Compare>::upper_bound(const Key& k) {
  return std::upper_bound(c_.begin(), c_.end(), k, KeyCompareWrapper{comp_});
}

template <typename Key, typename Value, typename Compare>
typename FlatMap<Key, Value, Compare>::const_iterator
FlatMap<Key, Value, Compare>::upper_bound(const Key& k) const {
  return std::upper_bound(c_.begin(), c_.end(), k, KeyCompareWrapper{comp_});
}

}  // namespace core

#endif  // CORE_FLAT_MAP_H_
