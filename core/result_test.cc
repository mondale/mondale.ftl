#include <sstream>

#include "base/basic_test.h"
#include "base/rawlog.h"
#include "core/result.h"

using core::BaseCode;
using core::Code;
using core::Result;
using core::ResultOr;

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
  EXPECT_TRUE(c.Is(Code::kOk));
  EXPECT_EQ(Code::kOk, c.base_code());
  EXPECT_EQ(ToString(c), ToString(Code::Ok()));
}

TEST(Code_ExplicitConstruct) {
  Code c(Code::kInvalidArgument);
  EXPECT_FALSE(IsOk(c));
  EXPECT_EQ(Code::kInvalidArgument, c.base_code());
  EXPECT_TRUE(c == Code::kInvalidArgument);
  EXPECT_TRUE(Code::kInvalidArgument == c);
  EXPECT_TRUE(c != Code::kError);
  EXPECT_TRUE(Code::kError != c);
}

Code TakesBaseReturnsCode(BaseCode bc) { return bc; }

TEST(Code_ImplicitConstruct) {
  EXPECT_TRUE(TakesBaseReturnsCode(Code::kError).Is(Code::kError));
}

TEST(Code_TrivialCopy) {
  Code c1(Code::kError);
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
  EXPECT_TRUE(StringsAreEqual(Code(Code::kOk)));
  EXPECT_TRUE(StringsAreEqual(Code(Code::kUnimplemented)));
}

TEST(Result_DefaultIsOk) {
  Result result;
  EXPECT_EQ(Code::Ok(), result.code());
  EXPECT_TRUE(result.IsOk());
  EXPECT_TRUE(IsOk(result));
  EXPECT_EQ(Code::kOk, result.base_code());
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
  const Result r(Code::kError);
  RAW_CHECK(!r.ErpEngaged());
  EXPECT_TRUE(StringsAreEqual(r));
  EXPECT_EQ(0, r.refs());
}

TEST(Result_Unengaged_NotAPointer) {
  const Result r(Code::kError);
  EXPECT_FALSE(r.ErpEngaged());
  EXPECT_LT(static_cast<int>(r.rep_bits()), 65536);
  EXPECT_EQ(0, r.refs());
}

Result ResultFromMove() { return Result(Code::kError); }

TEST(Result_Unengaged_MoveCtor) {
  Result moved = ResultFromMove();
  EXPECT_FALSE(moved.ErpEngaged());
  EXPECT_EQ(Code::kError, moved.base_code());
  EXPECT_EQ(0, moved.refs());
}

TEST(Result_Unengaged_MoveAssign) {
  Result from(Code::kError);
  Result to = std::move(from);
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(Code::kError, to.base_code());
  EXPECT_EQ(0, to.refs());

  // As it was unungaged, 'from' should still be viable.
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(Code::kError, from.base_code());
  EXPECT_EQ(0, from.refs());
}

TEST(Result_Unengaged_CopyCtor) {
  const Result from(Code::kError);
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(Code::kError, from.base_code());
  EXPECT_EQ(0, from.refs());

  const Result to(from);
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(Code::kError, to.base_code());
  EXPECT_EQ(0, to.refs());
}

TEST(Result_Unengaged_CopyAssign) {
  const Result from(Code::kError);
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(Code::kError, from.base_code());
  EXPECT_EQ(0, from.refs());

  const Result to = from;
  EXPECT_FALSE(to.ErpEngaged());
  EXPECT_EQ(Code::kError, to.base_code());
  EXPECT_EQ(0, to.refs());
}

TEST(Result_Engaged_String) {
  const Result r(Code::kError, "An error occurred!");
  ASSERT_TRUE(r.ErpEngaged());
  EXPECT_EQ(1, r.refs());
  EXPECT_EQ(std::string("kError//An error occurred!"), r.ToString());
}

Result EngagedResultFromMove() { return Result(Code::kError, "Oh noes!"); }

TEST(Result_Engaged_MoveCtor) {
  Result r(EngagedResultFromMove());
  ASSERT_TRUE(r.ErpEngaged());
  EXPECT_EQ(1, r.refs());
  EXPECT_EQ(std::string("kError//Oh noes!"), r.ToString());
}

TEST(Result_Engaged_MoveAssign) {
  Result from(Code::kUnimplemented, "Haven't done this yet.");
  ASSERT_TRUE(from.ErpEngaged());
  Result to = std::move(from);
  ASSERT_TRUE(to.ErpEngaged());
  EXPECT_EQ(Code::kUnimplemented, to.base_code());
  EXPECT_EQ(1, to.refs());

  // 'from' should not be clean and empty of the string.
  EXPECT_FALSE(from.ErpEngaged());
  EXPECT_EQ(Code::kUnimplemented, from.base_code());
}

TEST(Result_Engaged_CopyCtor) {
  const Result from(Code::kError, "Nope!");
  ASSERT_TRUE(from.ErpEngaged());
  EXPECT_EQ(Code::kError, from.base_code());
  EXPECT_EQ(1, from.refs());

  {
    const Result to(from);
    EXPECT_EQ(2, from.refs());
    ASSERT_TRUE(to.ErpEngaged());
    EXPECT_EQ(Code::kError, to.base_code());
    EXPECT_EQ(2, to.refs());
  }  // to drops its ref
  EXPECT_EQ(1, from.refs());
}

TEST(Result_Engaged_CopyAssign) {
  const Result from(Code::kError, "Nope!");
  ASSERT_TRUE(from.ErpEngaged());
  EXPECT_EQ(Code::kError, from.base_code());
  EXPECT_EQ(1, from.refs());

  {
    const Result to = from;
    EXPECT_EQ(2, from.refs());
    ASSERT_TRUE(to.ErpEngaged());
    EXPECT_EQ(Code::kError, to.base_code());
    EXPECT_EQ(2, to.refs());
  }  // to drops its ref
  EXPECT_EQ(1, from.refs());
}

TEST(ResultOr_ConstructFromValue) {
  ResultOr<int> ro(42);
  EXPECT_TRUE(ro.ok());
  EXPECT_EQ(42, ro.ValueOrDie());
  EXPECT_EQ(Code::kOk, ro.result().base_code());
}

TEST(ResultOr_ConstructFromUnengagedResult) {
  ResultOr<int> ro((Result(Code::kInvalidArgument)));
  EXPECT_FALSE(ro.ok());
  EXPECT_EQ(Code::kInvalidArgument, ro.result().base_code());
  EXPECT_FALSE(ro.result().ErpEngaged());
}

TEST(ResultOr_ConstructFromEngagedResult) {
  ResultOr<std::string> ro(Result(Code::kError, "File not found"));
  EXPECT_FALSE(ro.ok());
  EXPECT_EQ(Code::kError, ro.result().base_code());
  EXPECT_TRUE(ro.result().ErpEngaged());
  EXPECT_EQ(std::string("kError//File not found"), ro.result().ToString());
}

TEST(ResultOr_ImplicitConstructFromBaseCode) {
  ResultOr<int> ro(Code::kUnimplemented);
  EXPECT_FALSE(ro.ok());
  EXPECT_EQ(Code::kUnimplemented, ro.result().base_code());
}

TEST(ResultOr_CopyConstruct) {
  const ResultOr<std::string> original("Hello World");
  const ResultOr<std::string> copy(original);

  ASSERT_TRUE(copy.ok());
  EXPECT_EQ(std::string("Hello World"), copy.ValueOrDie());
  ASSERT_TRUE(original.ok());
  EXPECT_EQ(std::string("Hello World"), original.ValueOrDie());
}

TEST(ResultOr_CopyAssign) {
  const ResultOr<int> src(100);
  ResultOr<int> dest(Code::kError);

  dest = src;
  EXPECT_TRUE(dest.ok());
  EXPECT_EQ(100, dest.ValueOrDie());
  EXPECT_TRUE(src.ok());
}

TEST(ResultOr_MoveConstructValue) {
  ResultOr<std::string> src("Move Me");
  ResultOr<std::string> dest(std::move(src));

  EXPECT_TRUE(dest.ok());
  EXPECT_EQ(std::string("Move Me"), dest.ValueOrDie());
}

TEST(ResultOr_MoveAssignValue) {
  ResultOr<std::string> src("Move Me Too");
  ResultOr<std::string> dest(Code::kError);

  dest = std::move(src);
  EXPECT_TRUE(dest.ok());
  EXPECT_EQ(std::string("Move Me Too"), dest.ValueOrDie());
}

TEST(ResultOr_ValueOr) {
  const ResultOr<int> ok_ro(10);
  EXPECT_EQ(10, ok_ro.ValueOr(20));

  const ResultOr<int> err_ro(Code::kNotFound);
  EXPECT_EQ(20, err_ro.ValueOr(20));
}

TEST(ResultOr_InPlaceConstruct) {
  struct ComplexType {
    int a;
    std::string b;
    ComplexType(int a, std::string b) : a(a), b(std::move(b)) {}
  };

  ResultOr<ComplexType> ro(std::in_place, 5, "test");
  EXPECT_TRUE(ro.ok());
  EXPECT_EQ(5, ro.ValueOrDie().a);
  EXPECT_EQ(std::string("test"), ro.ValueOrDie().b);
}

TEST(TryWithBaseCode) {
  auto fn = []() -> BaseCode { return BaseCode::kError; };
  auto uut = [&]() -> Result {
    TRY(fn());
    RAW_FATAL << "Should not be reached.";
    return Result::Ok();
  };
  EXPECT_EQ(BaseCode::kError, uut().base_code());
}

TEST(TryWithCode) {
  auto fn = []() -> Code { return Code(Code::kError); };
  auto uut = [&]() -> Result {
    TRY(fn());
    RAW_FATAL << "Should not be reached.";
    return Result::Ok();
  };
  EXPECT_EQ(Code(Code::kError), uut().code());
}

TEST(TryWithResult) {
  auto fn = []() -> Result { return Result(Code::kError); };
  auto uut = [&]() -> Result {
    TRY(fn());
    RAW_FATAL << "Should not be reached.";
    return Result::Ok();
  };
  EXPECT_TRUE(uut().Is(Code::kError));
}

TEST(TryWithResultOr) {
  auto fn = []() -> Result { return Result(Code::kError); };
  auto uut = [&]() -> ResultOr<int> {
    TRY(fn());
    RAW_FATAL << "Should not be reached.";
    return 7;
  };
  EXPECT_TRUE(uut().result().Is(Code::kError));
}

TEST(TryAssignSuccess) {
  auto fn = []() -> ResultOr<int> { return 42; };
  auto uut = [&]() -> ResultOr<int> {
    TRY_ASSIGN(auto val, fn());
    return val + 8;
  };

  auto res = uut();
  ASSERT_TRUE(res.ok());
  EXPECT_EQ(50, res.ValueOrDie());
}

TEST(TryAssignExistingVariable) {
  auto fn = []() -> ResultOr<int> { return 100; };
  auto uut = [&]() -> Result {
    TRY_ASSIGN(int val, fn());
    EXPECT_EQ(100, val);
    return Result::Ok();
  };

  EXPECT_TRUE(uut().IsOk());
}

TEST(TryAssignPropagatesToResult) {
  auto fn = []() -> ResultOr<int> { return Result(Code::kError); };
  auto uut = [&]() -> Result {
    TRY_ASSIGN(auto val, fn());
    static_cast<void>(val);
    RAW_FATAL << "Should not be reached.";
    return Result::Ok();
  };

  EXPECT_TRUE(uut().Is(Code::kError));
}

TEST(TryAssignPropagatesToResultOr) {
  auto fn = []() -> ResultOr<int> { return Result(Code::kInvalidArgument); };
  auto uut = [&]() -> ResultOr<std::string> {
    TRY_ASSIGN(auto val, fn());
    RAW_FATAL << "Should not be reached.";
    return std::to_string(val);
  };

  auto res = uut();
  EXPECT_FALSE(res.ok());
  EXPECT_TRUE(res.result().Is(Code::kInvalidArgument));
}

TEST(TryAssignPropagatesToBaseCode) {
  auto fn = []() -> ResultOr<int> { return Result(BaseCode::kNotFound); };
  auto uut = [&]() -> BaseCode {
    TRY_ASSIGN(auto val, fn());
    RAW_FATAL << "Should not be reached.";
    static_cast<void>(val);
    return BaseCode::kOk;
  };

  EXPECT_EQ(BaseCode::kNotFound, uut());
}

TEST(TryAssignMoveOnlyType) {
  auto fn = []() -> ResultOr<std::unique_ptr<int>> {
    return std::make_unique<int>(42);
  };
  auto uut = [&]() -> ResultOr<int> {
    TRY_ASSIGN(auto ptr, fn());
    return *ptr;
  };

  auto res = uut();
  ASSERT_TRUE(res.ok());
  EXPECT_EQ(42, res.ValueOrDie());
}

}  // namespace
