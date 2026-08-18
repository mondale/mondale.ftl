#include "base/rawlog.h"
#include "core/result.h"

namespace core {

#define ENUM_SWITCH_TOSTRING(EnumType, Instance) \
  case EnumType::Instance:                       \
    return #Instance

namespace {
int ToInt(BaseCode bc) { return static_cast<int>(bc); }
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
  RAW_DCHECK(ErpEngaged());
  auto* const erp = rep.erp;
  erp->refs--;
  if (0 == erp->refs) {
    delete erp;
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
    ret += " ";
    ret += message();
  }
  return ret;
}

std::ostream& operator<<(std::ostream& out, const Result& r) {
  out << r.ToString();
  return out;
}

}  // namespace core
