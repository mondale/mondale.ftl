#include "core/inlined_vector.h"
#include "testing/testing.h"

namespace {

using core::InlinedVector;

struct DestructorTracker {
  int* destroy_count;

  explicit DestructorTracker(int* count) : destroy_count(count) {}

  ~DestructorTracker() {
    if (destroy_count != nullptr) {
      (*destroy_count)++;
    }
  }

  // Copy constructor
  DestructorTracker(const DestructorTracker& other)
      : destroy_count(other.destroy_count) {}

  // Move constructor: null out source's pointer so its destruction doesn't
  // double-count
  DestructorTracker(DestructorTracker&& other) noexcept
      : destroy_count(other.destroy_count) {
    other.destroy_count = nullptr;
  }

  DestructorTracker& operator=(const DestructorTracker&) = delete;
  DestructorTracker& operator=(DestructorTracker&&) = delete;
};

TEST(DefaultConstruction) {
  InlinedVector<int, 4> vec;
  EXPECT_TRUE(vec.empty());
  EXPECT_EQ(vec.size(), 0);
  EXPECT_EQ(vec.capacity(), 4);
}

TEST(PushBackAndAccess) {
  InlinedVector<int, 4> vec;
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);

  EXPECT_FALSE(vec.empty());
  EXPECT_EQ(vec.size(), 3);
  EXPECT_EQ(vec[0], 10);
  EXPECT_EQ(vec[1], 20);
  EXPECT_EQ(vec[2], 30);
}

TEST(InlineToHeapGrowth) {
  InlinedVector<int, 2> vec;
  vec.push_back(1);
  vec.push_back(2);
  EXPECT_EQ(vec.size(), 2);
  EXPECT_EQ(vec.capacity(), 2);

  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(5);

  EXPECT_EQ(vec.size(), 5);
  EXPECT_TRUE(vec.capacity() > 2);

  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[1], 2);
  EXPECT_EQ(vec[2], 3);
  EXPECT_EQ(vec[3], 4);
  EXPECT_EQ(vec[4], 5);
}

TEST(CopyConstructor) {
  InlinedVector<int, 2> vec1;
  vec1.push_back(42);
  vec1.push_back(84);

  InlinedVector<int, 2> vec2(vec1);
  EXPECT_EQ(vec2.size(), 2);
  EXPECT_EQ(vec2[0], 42);
  EXPECT_EQ(vec2[1], 84);
}

TEST(ClearVector) {
  InlinedVector<int, 4> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.clear();

  EXPECT_TRUE(vec.empty());
  EXPECT_EQ(vec.size(), 0);
}

TEST(DestructorLifetimes) {
  int destroy_counter = 0;

  {
    // Capacity N = 2. Push 3 elements to test both inline and heap storage
    // destructions.
    InlinedVector<DestructorTracker, 2> vec;
    vec.emplace_back(&destroy_counter);
    vec.emplace_back(&destroy_counter);
    vec.emplace_back(&destroy_counter);

    EXPECT_EQ(destroy_counter, 0);

    // Pop one element; its destructor should run immediately.
    vec.pop_back();
    EXPECT_EQ(destroy_counter, 1);

    // Clear remaining elements
    vec.clear();
    EXPECT_EQ(destroy_counter, 3);
  }

  // Once the vector goes out of scope, any remaining elements should be
  // destructed safely. (In this case, it was already cleared, so the final
  // count should remain at 3).
  EXPECT_EQ(destroy_counter, 3);
}

}  // namespace
