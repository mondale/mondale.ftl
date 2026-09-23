#include "io/source_buffer.h"
#include "testing/testing.h"

namespace io {

TEST(SourceBufferTest_InitializationAndConsume) {
  const char* text = "HighPerformanceIO";
  SourceBuffer buf(text, 17);

  EXPECT_EQ(buf.data(), text);
  EXPECT_EQ(buf.size(), 17);

  buf.Consume(4);  // Consume "High"
  EXPECT_EQ(buf.data(), text + 4);
  EXPECT_EQ(buf.size(), 13);
}

TEST(SourceBufferTest_MoveConstructor) {
  const char* text = "ZeroAllocation";
  SourceBuffer src(text, 14);

  // Move construct
  SourceBuffer dest(std::move(src));

  // Verify destination took ownership
  EXPECT_EQ(dest.data(), text);
  EXPECT_EQ(dest.size(), 14);

  // Verify source was safely nulled out
  EXPECT_EQ(src.data(), nullptr);
  EXPECT_EQ(src.size(), 0);
}

TEST(SourceBufferTest_MoveAssignment) {
  const char* text1 = "FirstBuffer";
  const char* text2 = "SecondBuffer";

  SourceBuffer buf1(text1, 11);
  SourceBuffer buf2(text2, 12);

  // Move assign
  buf1 = std::move(buf2);

  // Verify buf1 took over buf2's content
  EXPECT_EQ(buf1.data(), text2);
  EXPECT_EQ(buf1.size(), 12);

  // Verify buf2 was nulled out
  EXPECT_EQ(buf2.data(), nullptr);
  EXPECT_EQ(buf2.size(), 0);
}

TEST(SourceBufferTest_SelfMoveAssignmentSafety) {
  const char* text = "SelfAssignment";
  SourceBuffer buf(text, 14);

  // Suppress compiler warning for explicit self-move
  auto* p = &buf;
  buf = std::move(*p);

  // Should remain safely intact
  EXPECT_EQ(buf.data(), text);
  EXPECT_EQ(buf.size(), 14);
}

}  // namespace io
