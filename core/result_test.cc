#include <sstream>

#include "base/basic_test.h"
#include "base/rawlog.h"
#include "core/result.h"

using core::BaseCode;
using core::Code;
using core::Result;

namespace {

#define ENUM_EXPECT_STR(Enum)                           \
  do {                                                  \
    EXPECT_EQ(std::string_view(#Enum), ToString(Enum)); \
    std::stringstream ss;                               \
    ss << Enum;                                         \
    EXPECT_EQ(std::string(#Enum), ss.str());            \
  } while (0)

TEST(BaseCode_kOk) {
  using core::BaseCode::kOk;
  ENUM_EXPECT_STR(kOk);
  EXPECT_TRUE(IsOk(kOk));
}

TEST(BaseCode_kError) {
  using core::BaseCode::kError;
  ENUM_EXPECT_STR(kError);
  EXPECT_FALSE(IsOk(kError));
}

TEST(BaseCode_kInvalidArgument) {
  using core::BaseCode::kInvalidArgument;
  ENUM_EXPECT_STR(kInvalidArgument);
  EXPECT_FALSE(IsOk(kInvalidArgument));
}

TEST(BaseCode_kPermission) {
  using core::BaseCode::kPermission;
  ENUM_EXPECT_STR(kPermission);
  EXPECT_FALSE(IsOk(kPermission));
}

TEST(BaseCode_kCanceled) {
  using core::BaseCode::kCanceled;
  ENUM_EXPECT_STR(kCanceled);
  EXPECT_FALSE(IsOk(kCanceled));
}

TEST(BaseCode_kDeadline) {
  using core::BaseCode::kDeadline;
  ENUM_EXPECT_STR(kDeadline);
  EXPECT_FALSE(IsOk(kDeadline));
}

TEST(BaseCode_kNotFound) {
  using core::BaseCode::kNotFound;
  ENUM_EXPECT_STR(kNotFound);
  EXPECT_FALSE(IsOk(kNotFound));
}

TEST(BaseCode_kPrecondition) {
  using core::BaseCode::kPrecondition;
  ENUM_EXPECT_STR(kPrecondition);
  EXPECT_FALSE(IsOk(kPrecondition));
}

TEST(BaseCode_kExhausted) {
  using core::BaseCode::kExhausted;
  ENUM_EXPECT_STR(kExhausted);
  EXPECT_FALSE(IsOk(kExhausted));
}

TEST(BaseCode_kUnimplemented) {
  using core::BaseCode::kUnimplemented;
  ENUM_EXPECT_STR(kUnimplemented);
  EXPECT_FALSE(IsOk(kUnimplemented));
}

TEST(Code_DefaultOk) {
  Code c;
  EXPECT_EQ(c, Code::Ok());
  EXPECT_TRUE(IsOk(c));
  EXPECT_TRUE(c.IsOk());
  EXPECT_TRUE(c.Is(BaseCode::kOk));
  EXPECT_EQ(BaseCode::kOk, c.base_code());
  EXPECT_EQ(ToString(c), ToString(Code::Ok()));
}

TEST(Code_ExplicitConstruct) {
  Code c(BaseCode::kInvalidArgument);
  EXPECT_FALSE(IsOk(c));
  EXPECT_EQ(BaseCode::kInvalidArgument, c.base_code());
  EXPECT_TRUE(c == BaseCode::kInvalidArgument);
  EXPECT_TRUE(BaseCode::kInvalidArgument == c);
  EXPECT_TRUE(c != BaseCode::kError);
  EXPECT_TRUE(BaseCode::kError != c);
}

Code TakesBaseReturnsCode(BaseCode bc) { return bc; }

TEST(Code_ImplicitConstruct) {
  EXPECT_TRUE(TakesBaseReturnsCode(BaseCode::kError).Is(BaseCode::kError));
}

TEST(Code_TrivialCopy) {
  Code c1(BaseCode::kError);
  Code c2;
  c1 = c2;
  EXPECT_TRUE(IsOk(c1));
}

bool StringsAreEqual(Code c) {
  const auto bs = ToString(c.base_code());
  const auto s = ToString(c);
  std::stringstream ss;
  ss << c;
  return s == ss.str() && s == bs;
}

TEST(Code_Strings) {
  EXPECT_TRUE(StringsAreEqual(Code(BaseCode::kOk)));
  EXPECT_TRUE(StringsAreEqual(Code(BaseCode::kUnimplemented)));
}

TEST(Result_DefaultIsOk) {
  Result result;
  EXPECT_EQ(Code::Ok(), result.code());
  EXPECT_TRUE(result.IsOk());
  EXPECT_TRUE(IsOk(result));
  EXPECT_EQ(BaseCode::kOk, result.base_code());
  EXPECT_FALSE(result.ErpEngaged());
}

bool StringsAreEqual(const Result& r) {
  const auto bs = ToString(r.base_code());
  const auto s = r.ToString();
  std::stringstream ss;
  ss << r;
  return s == ss.str() && s == bs;
}

TEST(Result_Unengaged_ToString) {
  EXPECT_TRUE(StringsAreEqual(Result()));
  const Result r(BaseCode::kError);
  RAW_CHECK(!r.ErpEngaged());
  EXPECT_TRUE(StringsAreEqual(r));
}

TEST(Result_Unengaged_NotAPointer) {
  const Result r(BaseCode::kError);
  EXPECT_FALSE(r.ErpEngaged());
  EXPECT_LT(static_cast<int>(r.rep_bits()), 65536);
}

Result ResultFromMove() { return Result(BaseCode::kError); }

TEST(Result_Unengaged_MoveCtor) {
  Result moved = ResultFromMove();
  EXPECT_FALSE(moved.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, moved.base_code());
}

TEST(Result_Unengaged_MoveAssign) {
  Result from(BaseCode::kError);
  Result to = std::move(from);
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, to.base_code());

  // As it was unungaged, 'from' should still be viable.
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, from.base_code());
}

TEST(Result_Unengaged_CopyCtor) {
  const Result from(BaseCode::kError);
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, from.base_code());

  const Result to(from);
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, to.base_code());
}

TEST(Result_Unengaged_CopyAssign) {
  const Result from(BaseCode::kError);
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, from.base_code());

  const Result to = from;
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(BaseCode::kError, to.base_code());
}

}  // namespace
