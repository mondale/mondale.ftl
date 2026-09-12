#ifndef CAPSULE_TEXT_PARSING_WIDGET_H_
#define CAPSULE_TEXT_PARSING_WIDGET_H_

#include <functional>
#include <map>
#include <string_view>
#include <vector>

#include "capsule/text/text_parser.h"

namespace capsule::text {

// Helper to keep the parsing state machine as outside of generated types as
// possible.
class ParsingWidget final {
 public:
  using ParseFn = std::function<Result(capsule::text::TextParser* p)>;

  void Add(std::string_view n, bool* bp);
  void Add(std::string_view n, uint8_t* u8p);
  void Add(std::string_view n, int8_t* i8p);
  void Add(std::string_view n, uint16_t* u16p);
  void Add(std::string_view n, int16_t* i16p);
  void Add(std::string_view n, uint32_t* u32p);
  void Add(std::string_view n, int32_t* i32p);
  void Add(std::string_view n, float* f32p);
  void Add(std::string_view n, uint64_t* u64p);
  void Add(std::string_view n, int64_t* i64p);
  void Add(std::string_view n, double* f64p);
  void Add(std::string_view n, std::string* sp);
  void AddStringVector(std::string_view n, std::vector<std::string>* vsp);

  // The target of this ParseFn is ParseFrom on the nested capsule.
  void AddCapsule(std::string_view n, ParseFn fn);

  // The target of this ParseFn is a method that invokes emplace_back on the
  // target vector then invokes ParseFrom on the nested capsule.
  void AddCapsuleVector(std::string_view n, ParseFn fn);

  Result ParseFrom(::capsule::text::TextParser* p);

 private:
  Result ParseSubcapsule(std::string_view n, capsule::text::TextParser* p);
  Result ParseVector(std::string_view n, capsule::text::TextParser* p);
  Result ParsePrimitive(std::string_view n, capsule::text::TextParser* p);
  Result ParseStringVector(capsule::text::TextParser* p,
                           std::vector<std::string>* v);

  std::map<std::string_view, bool*> bm_;
  std::map<std::string_view, uint8_t*> u8m_;
  std::map<std::string_view, int8_t*> i8m_;
  std::map<std::string_view, uint16_t*> u16m_;
  std::map<std::string_view, int16_t*> i16m_;
  std::map<std::string_view, uint32_t*> u32m_;
  std::map<std::string_view, int32_t*> i32m_;
  std::map<std::string_view, float*> f32m_;
  std::map<std::string_view, uint64_t*> u64m_;
  std::map<std::string_view, int64_t*> i64m_;
  std::map<std::string_view, double*> f64m_;
  std::map<std::string_view, std::string*> string_m_;
  std::map<std::string_view, std::vector<std::string>*> string_vector_m_;
  std::map<std::string_view, ParseFn> capsule_m_;
  std::map<std::string_view, ParseFn> capsule_vector_m_;
};

}  // namespace capsule::text

#endif  // #ifndef CAPSULE_TEXT_PARSING_WIDGET_H_
