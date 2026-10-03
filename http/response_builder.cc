#include <vector>

#include "http/response_builder.h"

namespace {

void WriteAttributes(
    std::string& buf,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        attrs) {
  for (const auto& [k, v] : attrs) {
    buf.push_back(' ');
    buf.append(k);
    buf.append("=\"");
    buf.append(v);
    buf.push_back('"');
  }
}

}  // namespace

namespace http {

class ResponseBuilder::Impl {
 public:
  int status_code = 200;
  std::string status_message = "OK";
  std::vector<std::pair<std::string, std::string>> headers;

  std::string html_buffer;
  int indent_level = 0;

  void WriteIndent() { html_buffer.append(indent_level * 2, ' '); }

  void OpenTag(
      std::string_view name,
      std::initializer_list<std::pair<std::string_view, std::string_view>>
          attrs) {
    WriteIndent();
    html_buffer.push_back('<');
    html_buffer.append(name);
    WriteAttributes(html_buffer, attrs);
    html_buffer.append(">\n");
    ++indent_level;
  }

  void CloseTag(std::string_view name) {
    --indent_level;
    WriteIndent();
    html_buffer.append("</");
    html_buffer.append(name);
    html_buffer.append(">\n");
  }

  void WriteLeaf(
      std::string_view name, std::string_view text,
      std::initializer_list<std::pair<std::string_view, std::string_view>>
          attrs) {
    WriteIndent();
    html_buffer.push_back('<');
    html_buffer.append(name);
    WriteAttributes(html_buffer, attrs);
    html_buffer.push_back('>');
    html_buffer.append(text);
    html_buffer.append("</");
    html_buffer.append(name);
    html_buffer.append(">\n");
  }

  void WriteSelfClosing(
      std::string_view name,
      std::initializer_list<std::pair<std::string_view, std::string_view>>
          attrs) {
    WriteIndent();
    html_buffer.push_back('<');
    html_buffer.append(name);
    WriteAttributes(html_buffer, attrs);
    html_buffer.append(" />\n");
  }
};

ResponseBuilder::ResponseBuilder() : impl_(std::make_unique<Impl>()) {}

ResponseBuilder::~ResponseBuilder() = default;

void ResponseBuilder::SetStatus(int code, std::string_view message) {
  impl_->status_code = code;
  impl_->status_message = message;
}

void ResponseBuilder::SetHeader(std::string_view name, std::string_view value) {
  impl_->headers.emplace_back(std::string(name), std::string(value));
}

std::string ResponseBuilder::ToString() const {
  std::string response;

  // 1. Status line
  response.append("HTTP/1.1 ");
  response.append(std::to_string(impl_->status_code));
  response.push_back(' ');
  response.append(impl_->status_message);
  response.append("\r\n");

  // 2. Custom headers
  for (const auto& [name, value] : impl_->headers) {
    response.append(name);
    response.append(": ");
    response.append(value);
    response.append("\r\n");
  }

  // 3. Automatic Content-Length
  response.append("Content-Length: ");
  response.append(std::to_string(impl_->html_buffer.size()));
  response.append("\r\n\r\n");

  // 4. HTML Body
  response.append(impl_->html_buffer);

  return response;
}

void ResponseBuilder::Doctype(std::string_view type) {
  impl_->WriteIndent();
  impl_->html_buffer.append("<!DOCTYPE ");
  impl_->html_buffer.append(type);
  impl_->html_buffer.append(">\n");
}

void ResponseBuilder::Html(std::move_only_function<void()> cb) {
  Html({}, std::move(cb));
}

void ResponseBuilder::Html(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("html", a);
  cb();
  impl_->CloseTag("html");
}

void ResponseBuilder::Head(std::move_only_function<void()> cb) {
  impl_->OpenTag("head", {});
  cb();
  impl_->CloseTag("head");
}

void ResponseBuilder::Body(std::move_only_function<void()> cb) {
  impl_->OpenTag("body", {});
  cb();
  impl_->CloseTag("body");
}

void ResponseBuilder::Header(std::move_only_function<void()> cb) {
  impl_->OpenTag("header", {});
  cb();
  impl_->CloseTag("header");
}

void ResponseBuilder::Div(std::move_only_function<void()> cb) {
  impl_->OpenTag("div", {});
  cb();
  impl_->CloseTag("div");
}

void ResponseBuilder::Div(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("div", a);
  cb();
  impl_->CloseTag("div");
}

void ResponseBuilder::H1(std::string_view t) { impl_->WriteLeaf("h1", t, {}); }
void ResponseBuilder::H2(std::string_view t) { impl_->WriteLeaf("h2", t, {}); }
void ResponseBuilder::H3(std::string_view t) { impl_->WriteLeaf("h3", t, {}); }

void ResponseBuilder::P(std::move_only_function<void()> cb) {
  impl_->OpenTag("p", {});
  cb();
  impl_->CloseTag("p");
}

void ResponseBuilder::P(std::string_view t) { impl_->WriteLeaf("p", t, {}); }

void ResponseBuilder::Span(std::string_view t) {
  impl_->WriteLeaf("span", t, {});
}
void ResponseBuilder::Span(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("span", t, a);
}

void ResponseBuilder::A(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("a", t, a);
}

void ResponseBuilder::A(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("a", a);
  cb();
  impl_->CloseTag("a");
}

void ResponseBuilder::Ul(std::move_only_function<void()> cb) {
  impl_->OpenTag("ul", {});
  cb();
  impl_->CloseTag("ul");
}

void ResponseBuilder::Li(std::string_view t) { impl_->WriteLeaf("li", t, {}); }

void ResponseBuilder::Table(std::move_only_function<void()> cb) {
  impl_->OpenTag("table", {});
  cb();
  impl_->CloseTag("table");
}

void ResponseBuilder::Table(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("table", a);
  cb();
  impl_->CloseTag("table");
}

void ResponseBuilder::Tr(std::move_only_function<void()> cb) {
  impl_->OpenTag("tr", {});
  cb();
  impl_->CloseTag("tr");
}

void ResponseBuilder::Th(std::string_view t) { impl_->WriteLeaf("th", t, {}); }
void ResponseBuilder::Td(std::string_view t) { impl_->WriteLeaf("td", t, {}); }
void ResponseBuilder::Td(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("td", t, a);
}

void ResponseBuilder::Form(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("form", a);
  cb();
  impl_->CloseTag("form");
}

void ResponseBuilder::Label(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("label", t, a);
}

void ResponseBuilder::Input(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a) {
  impl_->WriteSelfClosing("input", a);
}

void ResponseBuilder::Button(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("button", t, a);
}

void ResponseBuilder::Button(std::string_view t) {
  impl_->WriteLeaf("button", t, {});
}

void ResponseBuilder::Title(std::string_view t) {
  impl_->WriteLeaf("title", t, {});
}

void ResponseBuilder::Link(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a) {
  impl_->WriteSelfClosing("link", a);
}

void ResponseBuilder::Meta(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a) {
  impl_->WriteSelfClosing("meta", a);
}

void ResponseBuilder::Style(std::string_view css) {
  impl_->WriteLeaf("style", css, {});
}

void ResponseBuilder::Text(std::string_view t) {
  impl_->WriteIndent();
  impl_->html_buffer.append(t);
  impl_->html_buffer.push_back('\n');
}

void ResponseBuilder::Raw(std::string_view html) {
  impl_->WriteIndent();
  impl_->html_buffer.append(html);
  impl_->html_buffer.push_back('\n');
}

}  // namespace http
