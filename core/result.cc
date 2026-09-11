#include <string.h>

#include <memory>
#include <string>

#include "base/rawlog.h"
#include "core/result.h"

namespace core {

#define ENUM_SWITCH_TOSTRING(EnumType, Instance) \
  case EnumType::Instance:                       \
    return #Instance

namespace {

int ToInt(BaseCode bc) { return static_cast<int>(bc); }

Result MakeError(Code c, std::string_view msg, base::SourceLocation loc) {
  return Result(
      c, std::format("{}:{} // {}", loc.relative_file_name(), loc.line(), msg));
}

}  // namespace

std::string_view ToString(BaseCode bc) {
  RAW_DCHECK(ToInt(bc) >= ToInt(BaseCode::kOk)) << ToInt(bc);
  RAW_DCHECK(ToInt(bc) <= ToInt(BaseCode::kUnimplemented)) << ToInt(bc);
  switch (bc) {
    ENUM_SWITCH_TOSTRING(BaseCode, kOk);
    ENUM_SWITCH_TOSTRING(BaseCode, kError);
    ENUM_SWITCH_TOSTRING(BaseCode, kInvalidArgument);
    ENUM_SWITCH_TOSTRING(BaseCode, kPermission);
    ENUM_SWITCH_TOSTRING(BaseCode, kCanceled);
    ENUM_SWITCH_TOSTRING(BaseCode, kDeadline);
    ENUM_SWITCH_TOSTRING(BaseCode, kNotFound);
    ENUM_SWITCH_TOSTRING(BaseCode, kPrecondition);
    ENUM_SWITCH_TOSTRING(BaseCode, kExhausted);
    ENUM_SWITCH_TOSTRING(BaseCode, kUnavailable);
    ENUM_SWITCH_TOSTRING(BaseCode, kCapsuleFatal);
    ENUM_SWITCH_TOSTRING(BaseCode, kStreamFatal);
    ENUM_SWITCH_TOSTRING(BaseCode, kEintr);
    ENUM_SWITCH_TOSTRING(BaseCode, kEnoent);
    ENUM_SWITCH_TOSTRING(BaseCode, kEinval);
    ENUM_SWITCH_TOSTRING(BaseCode, kUnimplemented);
  }
}

std::ostream& operator<<(std::ostream& out, BaseCode bc) {
  out << ToString(bc);
  return out;
}

std::string_view ToString(Code c) { return ToString(c.base_code()); }

std::ostream& operator<<(std::ostream& out, Code c) {
  out << ToString(c);
  return out;
}

void Result::DropRef() {
  std::unique_ptr<ExtendedRep> deleter;
  RAW_DCHECK(ErpEngaged());
  auto* const erp = rep.erp;
  erp->refs--;
  if (0 == erp->refs) {
    deleter.reset(erp);
  }
}

void Result::Disengage() {
  RAW_DCHECK(ErpEngaged()) << std::hex << rep.bits;
  auto* const erp = rep.erp;
  rep.code = erp->code;
  RAW_DCHECK(!ErpEngaged()) << std::hex << rep.bits;
}

Result::ExtendedRep::ExtendedRep(Code c, std::string_view m)
    : refs(1), code(c), message(m) {}

Result::Result(Code c, std::string_view m) { rep.erp = new ExtendedRep(c, m); }

std::string_view Result::message() const {
  if (ErpEngaged()) {
    return rep.erp->message;
  }
  return "";
}

std::string Result::ToString() const {
  std::string ret(::core::ToString(code()));
  if (ErpEngaged()) {
    ret += "//";
    ret += message();
  }
  return ret;
}

std::ostream& operator<<(std::ostream& out, const Result& r) {
  out << r.ToString();
  return out;
}

BaseCode BaseCodeFromErrno(int e) {
  switch (e) {
    case EINTR:
      return BaseCode::kEintr;
    case ENOENT:
      return BaseCode::kEnoent;
    case EINVAL:
      return BaseCode::kEinval;
    default:
      break;
  }
  RAW_WARNING << "Unhandled Errno coercing to kUnimplemented["
              << strerrorname_np(e) << "]";
  return BaseCode::kUnimplemented;
}

Code CodeFromErrno(int e) { return BaseCodeFromErrno(e); }
Result ResultFromErrno(int e) {
  const auto bc = BaseCodeFromErrno(e);
  if (bc == BaseCode::kUnimplemented) {
    std::string message = "Unhandled Errno[";
    message += strerrorname_np(e);
    message += "]";
    return Result(bc, message);
  }
  return Result(bc);
}

Result InvalidArgumentError(std::string_view msg, base::SourceLocation loc) {
  return MakeError(Code::kInvalidArgument, msg, loc);
}

Result NotFoundError(std::string_view msg, base::SourceLocation loc) {
  return MakeError(Code::kNotFound, msg, loc);
}

Result PermissionError(std::string_view msg, base::SourceLocation loc) {
  return MakeError(Code::kPermission, msg, loc);
}

Result CapsuleFatalError(std::string_view msg, base::SourceLocation loc) {
  return MakeError(Code::kCapsuleFatal, msg, loc);
}

Result UnimplementedError(std::string_view msg, base::SourceLocation loc) {
  return MakeError(Code::kUnimplemented, msg, loc);
}

}  // namespace core
