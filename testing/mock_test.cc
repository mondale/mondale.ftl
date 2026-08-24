#include "testing/testing.h"

namespace {

class Thing {
 public:
  virtual ~Thing() = default;

  virtual void Zero() = 0;
  virtual int One(int a1) = 0;
  virtual std::string Two(int a1, int a2) const = 0;
  virtual int Three(int a1, int a2, int a3) = 0;
  virtual int Four(int a1, int a2, int a3, int a4) = 0;
  virtual int Five(int a1, int a2, int a3, int a4, int a5) = 0;
  virtual int Six(int a1, int a2, int a3, int a4, int a5, int a6) = 0;
  virtual int Seven(int a1, int a2, int a3, int a4, int a5, int a6, int a7) = 0;
  virtual int Eight(int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                    int a8) = 0;
  virtual int Nine(int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                   int a8, int a9) = 0;
  virtual int Ten(int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                  int a8, int a9, int a10) = 0;
};

class MockThing : public Thing {
 public:
  MOCK_METHOD0(void, Zero, override);
  MOCK_METHOD1(int, One, int, override);
  MOCK_METHOD2(std::string, Two, int, int, const override);
  MOCK_METHOD3(int, Three, int, int, int, override);
  MOCK_METHOD4(int, Four, int, int, int, int, override);
  MOCK_METHOD5(int, Five, int, int, int, int, int, override);
  MOCK_METHOD6(int, Six, int, int, int, int, int, int, override);
  MOCK_METHOD7(int, Seven, int, int, int, int, int, int, int, override);
  MOCK_METHOD8(int, Eight, int, int, int, int, int, int, int, int, override);
  MOCK_METHOD9(int, Nine, int, int, int, int, int, int, int, int, int,
               override);
  MOCK_METHOD10(int, Ten, int, int, int, int, int, int, int, int, int, int,
                override);
};

TEST(MockTestCallWithReturnArity0) {
  MockThing m;
  EXPECT_CALL(m, Zero()).WillOnce([]() { return; });
  m.Zero();
}

TEST(MockTestCallWithReturnArity1) {
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 8; });
  EXPECT_EQ(8, m.One(7));
}

TEST(MockTestCallWithReturnArity2) {
  MockThing m;
  EXPECT_CALL(m, Two(1, 2)).WillOnce([](int, int) { return "matched"; });
  EXPECT_EQ("matched", m.Two(1, 2));
}

TEST(MockTestCallWithReturnArity3) {
  MockThing m;
  EXPECT_CALL(m, Three(1, 2, 3)).WillOnce([](int, int, int) { return 300; });
  EXPECT_EQ(300, m.Three(1, 2, 3));
}

TEST(MockTestCallWithReturnArity4) {
  MockThing m;
  EXPECT_CALL(m, Four(1, 2, 3, 4)).WillOnce([](int, int, int, int) {
    return 400;
  });
  EXPECT_EQ(400, m.Four(1, 2, 3, 4));
}

TEST(MockTestCallWithReturnArity5) {
  MockThing m;
  EXPECT_CALL(m, Five(1, 2, 3, 4, 5)).WillOnce([](int, int, int, int, int) {
    return 500;
  });
  EXPECT_EQ(500, m.Five(1, 2, 3, 4, 5));
}

TEST(MockTestCallWithReturnArity6) {
  MockThing m;
  EXPECT_CALL(m, Six(1, 2, 3, 4, 5, 6))
      .WillOnce([](int, int, int, int, int, int) { return 600; });
  EXPECT_EQ(600, m.Six(1, 2, 3, 4, 5, 6));
}

TEST(MockTestCallWithReturnArity7) {
  MockThing m;
  EXPECT_CALL(m, Seven(1, 2, 3, 4, 5, 6, 7))
      .WillOnce([](int, int, int, int, int, int, int) { return 700; });
  EXPECT_EQ(700, m.Seven(1, 2, 3, 4, 5, 6, 7));
}

TEST(MockTestCallWithReturnArity8) {
  MockThing m;
  EXPECT_CALL(m, Eight(1, 2, 3, 4, 5, 6, 7, 8))
      .WillOnce([](int, int, int, int, int, int, int, int) { return 800; });
  EXPECT_EQ(800, m.Eight(1, 2, 3, 4, 5, 6, 7, 8));
}

TEST(MockTestCallWithReturnArity9) {
  MockThing m;
  EXPECT_CALL(m, Nine(1, 2, 3, 4, 5, 6, 7, 8, 9))
      .WillOnce(
          [](int, int, int, int, int, int, int, int, int) { return 900; });
  EXPECT_EQ(900, m.Nine(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(MockTestCallWithReturnArity10) {
  MockThing m;
  EXPECT_CALL(m, Ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10))
      .WillOnce([](int, int, int, int, int, int, int, int, int, int) {
        return 1000;
      });
  EXPECT_EQ(1000, m.Ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(MockTestNonMatchingCall) {
  ExpectFailure();
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 8; });
  EXPECT_EQ(0, m.One(6));
}

TEST(MockTestNoCallAtAll) {
  ExpectFailure();
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 8; });
}

TEST(MockTestMultipleWillOnceSameArgs) {
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 8; });
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 9; });

  // WillOnce() Expectations match inorder! And automatically retire on
  // saturation.
  EXPECT_EQ(8, m.One(7));
  EXPECT_EQ(9, m.One(7));
}

TEST(MockTestMultipleWillOnceDiffArgs) {
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 7; });
  EXPECT_CALL(m, One(8)).WillOnce([](int x) { return 8; });

  EXPECT_EQ(8, m.One(8));
  EXPECT_EQ(7, m.One(7));
}

TEST(MockTestWillOnceAndRepeatedly) {
  MockThing m;
  EXPECT_CALL(m, One(7)).WillOnce([](int x) { return 1; });
  EXPECT_CALL(m, One(8)).WillOnce([](int x) { return 1; });
  EXPECT_CALL(m, One(testing::_)).WillRepeatedly([](int x) { return x; });

  EXPECT_EQ(1, m.One(1));
  EXPECT_EQ(2, m.One(2));
  EXPECT_EQ(1, m.One(7));
  EXPECT_EQ(1, m.One(8));
  EXPECT_EQ(9, m.One(9));
}

}  // namespace
