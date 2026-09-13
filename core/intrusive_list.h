#ifndef CORE_INTRUSIVE_LIST_H_
#define CORE_INTRUSIVE_LIST_H_

// Provides an intrusive doubly-linked list supporting multi-list membership via
// tags and blind element erasure. Objects must derive from
//
//  struct Task : public IntrusiveListHook<struct PriorityTag>,
//                public IntrusiveListHook<struct OwnerTag> {
//    int id;
//    explicit Task(int task_id) : id(task_id) {}
//  };
//
//   IntrusiveList<Task, PriorityTag> high_priority_list;
//   IntrusiveList<Task, OwnerTag> worker_pool_list;
//
//   Task task_alpha(101);
//   Task task_beta(102);
//
//   // Place task_alpha in both lists using distinct tags
//   high_priority_list.PushBack(&task_alpha);
//   worker_pool_list.PushBack(&task_alpha);
//
//   // task_beta is only in the worker pool
//   worker_pool_list.PushBack(&task_beta);
//
//   // Blind erase task_alpha from the priority list using only its pointer
//   IntrusiveList<Task, PriorityTag>::Erase(&task_alpha);

#include <iterator>
#include <type_traits>

#include "core/vocabulary.h"

namespace core {

// Hook class that enables a type to be intrusive-linked into an IntrusiveList.
template <typename Tag = void>
class IntrusiveListHook {
 public:
  // Initializes an unlinked hook instance.
  IntrusiveListHook() : prev_(nullptr), next_(nullptr) {}

  // Automatically unlinks the node from any containing list upon destruction.
  ~IntrusiveListHook() { Unlink(); }

  IntrusiveListHook(const IntrusiveListHook&) = delete;
  IntrusiveListHook& operator=(const IntrusiveListHook&) = delete;
  IntrusiveListHook(IntrusiveListHook&&) = delete;
  IntrusiveListHook& operator=(IntrusiveListHook&&) = delete;

  // Returns true if this hook is currently linked in a list.
  bool IsLinked() const { return prev_ != nullptr; }

  // Removes this hook from its current list, making it unlinked.
  void Unlink() {
    if (!IsLinked()) {
      return;
    }
    prev_->next_ = next_;
    next_->prev_ = prev_;
    prev_ = nullptr;
    next_ = nullptr;
  }

 private:
  template <typename U, typename TagParam>
  friend class IntrusiveList;

  IntrusiveListHook* prev_{nullptr};
  IntrusiveListHook* next_{nullptr};
};

// Doubly-linked intrusive list container for types derived from
// IntrusiveListHook<Tag>.
template <typename T, typename Tag = void>
class IntrusiveList {
 public:
  static_assert(std::is_base_of_v<IntrusiveListHook<Tag>, T>,
                "T must derive from IntrusiveListHook<Tag>");

  // Constructs an empty intrusive list.
  IntrusiveList() {
    sentinel_.next_ = &sentinel_;
    sentinel_.prev_ = &sentinel_;
  }

  // Destroys the list, unlinking remaining elements (does not delete objects).
  ~IntrusiveList() { Clear(); }

  IntrusiveList(const IntrusiveList&) = delete;
  IntrusiveList& operator=(const IntrusiveList&) = delete;

  IntrusiveList(IntrusiveList&& o) { MoveFrom(std::move(o)); }

  IntrusiveList& operator=(IntrusiveList&& o) {
    if (this != &o) {
      Clear();
      MoveFrom(std::move(o));
    }
    return *this;
  }

  // Returns true if the given element is currently linked in this list.
  bool IsLinked(const T* x) const {
    const auto* hook = static_cast<const IntrusiveListHook<Tag>*>(x);
    return hook->IsLinked();
  }

  // Returns true if the list contains no elements.
  bool Empty() const { return sentinel_.next_ == &sentinel_; }

  // Appends an element to the back of the list.
  void PushBack(T* x) { InsertBefore(&sentinel_, x); }

  // Prepends an element to the front of the list.
  void PushFront(T* x) { InsertAfter(&sentinel_, x); }

  // Removes the element at the back of the list.
  void PopBack() {
    DCHECK(!Empty());
    Remove(static_cast<T*>(sentinel_.prev_));
  }

  // Removes the element at the front of the list.
  void PopFront() {
    DCHECK(!Empty());
    Remove(static_cast<T*>(sentinel_.next_));
  }

  // Removes all elements from the list.
  void Clear() {
    while (!Empty()) {
      PopFront();
    }
  }

  // Blindly erases an object from whatever list its hook belongs to.
  static void Erase(T* x) {
    auto* hook = static_cast<IntrusiveListHook<Tag>*>(x);
    DCHECK(hook->IsLinked());
    hook->Unlink();
  }

  // Bidirectional iterator for traversing the intrusive list.
  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit Iterator(IntrusiveListHook<Tag>* p) : ptr_(p) {}

    T& operator*() const { return *static_cast<T*>(ptr_); }

    T* operator->() const { return static_cast<T*>(ptr_); }

    Iterator& operator++() {
      ptr_ = ptr_->next_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ptr_ = ptr_->next_;
      return tmp;
    }

    Iterator& operator--() {
      ptr_ = ptr_->prev_;
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      ptr_ = ptr_->prev_;
      return tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.ptr_ == b.ptr_;
    }

    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return a.ptr_ != b.ptr_;
    }

   private:
    IntrusiveListHook<Tag>* ptr_;
  };

  // Returns an iterator to the beginning of the list.
  Iterator begin() { return Iterator(sentinel_.next_); }

  // Returns an iterator to the end of the list.
  Iterator end() { return Iterator(&sentinel_); }

  // Returns a const iterator to the beginning of the list.
  Iterator begin() const {
    return Iterator(const_cast<IntrusiveListHook<Tag>*>(sentinel_.next_));
  }

  // Returns a const iterator to the end of the list.
  Iterator end() const {
    return Iterator(const_cast<IntrusiveListHook<Tag>*>(&sentinel_));
  }

 private:
  void InsertBefore(IntrusiveListHook<Tag>* pos, T* x) {
    auto* hook = static_cast<IntrusiveListHook<Tag>*>(x);
    DCHECK(!hook->IsLinked());

    auto* prev = pos->prev_;
    hook->prev_ = prev;
    hook->next_ = pos;
    prev->next_ = hook;
    pos->prev_ = hook;
  }

  void InsertAfter(IntrusiveListHook<Tag>* pos, T* x) {
    auto* hook = static_cast<IntrusiveListHook<Tag>*>(x);
    DCHECK(!hook->IsLinked());

    auto* next = pos->next_;
    hook->prev_ = pos;
    hook->next_ = next;
    pos->next_ = hook;
    next->prev_ = hook;
  }

  void Remove(T* x) {
    auto* hook = static_cast<IntrusiveListHook<Tag>*>(x);
    DCHECK(hook->IsLinked());
    hook->Unlink();
  }

  void MoveFrom(IntrusiveList&& o) {
    if (o.Empty()) {
      sentinel_.next_ = &sentinel_;
      sentinel_.prev_ = &sentinel_;
    } else {
      sentinel_.next_ = o.sentinel_.next_;
      sentinel_.prev_ = o.sentinel_.prev_;
      sentinel_.next_->prev_ = &sentinel_;
      sentinel_.prev_->next_ = &sentinel_;
      o.sentinel_.next_ = &o.sentinel_;
      o.sentinel_.prev_ = &o.sentinel_;
    }
  }

  IntrusiveListHook<Tag> sentinel_;
};

}  // namespace core

#endif  // #ifndef CORE_INTRUSIVE_LIST_H_
