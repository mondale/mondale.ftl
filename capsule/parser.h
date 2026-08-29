#ifndef CAPSULE_PARSER_H_
#define CAPSULE_PARSER_H_

#include <string>
#include <string_view>
#include <vector>

#include "capsule/ast.h"
#include "capsule/lexer.h"
#include "core/vocabulary.h"

namespace capsule {

class Parser {
 public:
  Parser(std::string_view src, std::string_view filename);
  ~Parser();

  ResultOr<CapsuleFile> Parse();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace capsule

#endif  // CAPSULE_PARSER_H_
