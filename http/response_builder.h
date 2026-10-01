#ifndef HTTP_RESPONSE_BUILDER_H_
#define HTTP_RESPONSE_BUILDER_H_

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "core/vocabulary.h"

namespace http {

class HttpResponseBuilder final {
 public:
  HttpResponseBuilder();
  ~HttpResponseBuilder();

  // HTTP Protocol Configuration
  void SetStatus(int code, std::string_view message);
  void SetHeader(std::string_view name, std::string_view value);

  // Assembles and returns the full HTTP/1.1 response (Headers + \r\n\r\n + HTML
  // Body).
  std::string ToString() const;

  // Structural and container elements
  void Html(std::move_only_function<void()> cb);
  void Head(std::move_only_function<void()> cb);
  void Body(std::move_only_function<void()> cb);
  void Header(std::move_only_function<void()> cb);
  void Div(std::move_only_function<void()> cb);
  void Div(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::move_only_function<void()> cb);

  // Typography and headings
  void H1(std::string_view t);
  void H2(std::string_view t);
  void H3(std::string_view t);
  void P(std::move_only_function<void()> cb);
  void Span(std::string_view t);
  void Span(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::string_view t);

  // Lists
  void Ul(std::move_only_function<void()> cb);
  void Li(std::string_view t);

  // Tables
  void Table(std::move_only_function<void()> cb);
  void Table(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::move_only_function<void()> cb);
  void Tr(std::move_only_function<void()> cb);
  void Th(std::string_view t);
  void Td(std::string_view t);
  void Td(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::string_view t);

  // Forms and Inputs
  void Form(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::move_only_function<void()> cb);
  void Label(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::string_view t);
  void Input(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a);
  void Button(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a,
      std::string_view t);
  void Button(std::string_view t);

  // Head metadata, inline text, and raw injection
  void Title(std::string_view t);
  void Link(
      std::initializer_list<std::pair<std::string_view, std::string_view>> a);
  void Text(std::string_view t);
  void Raw(std::string_view html);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace http

#endif  // HTTP_RESPONSE_BUILDER_H_
