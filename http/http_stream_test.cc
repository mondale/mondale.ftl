#include "http/http_stream.h"
#include "io/io.h"
#include "testing/testing.h"

using testing::IsOk;

namespace http {

class HttpStreamTest : public ::testing::Test, public io::Poster {
 protected:
  void Post(io::SourceBuffer sb) override { Log(FATAL) << "Illegal post."; }
  void Post(io::SinkBuffer sb) override { sinks_.push_back(std::move(sb)); }
  void PostClose() override { close_requested_ = true; }

  bool close_requested_ = false;
  std::list<io::SinkBuffer> sinks_;
};

TEST_F(HttpStreamTest, BasicParse) {
  constexpr char kMinimal[] = "GET /hola HTTP/1.1\r\nHost: localhost\r\n\r\n";
  bool called = false;
  HttpStream hs(
      this, [&](Result r, const RequestParser& rp, io::Poster* p) -> Result {
        EXPECT_THAT(r, IsOk());
        EXPECT_EQ(p, this);
        EXPECT_EQ(RequestParser::Method::kGet, rp.method());
        EXPECT_EQ(RequestParser::Version::kHttp1point1, rp.version());
        called = true;
        return Result::Ok();
      });
  hs.Prime();
  EXPECT_FALSE(close_requested_);
  ASSERT_TRUE(!sinks_.empty());
  ASSERT_GE(sinks_.front().size(), sizeof(kMinimal));
  memcpy(sinks_.front().data(), kMinimal, sizeof(kMinimal));
  sinks_.front().Advance(sizeof(kMinimal));
  sinks_.pop_front();

  EXPECT_TRUE(called);
}

// TODO - set flags smaller and try buffer shennanigans.

}  // namespace http
