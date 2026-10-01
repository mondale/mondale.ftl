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

class HttpResponseBuilder::Impl {
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

HttpResponseBuilder::HttpResponseBuilder() : impl_(std::make_unique<Impl>()) {}

HttpResponseBuilder::~HttpResponseBuilder() = default;

void HttpResponseBuilder::SetStatus(int code, std::string_view message) {
  impl_->status_code = code;
  impl_->status_message = message;
}

void HttpResponseBuilder::SetHeader(std::string_view name,
                                    std::string_view value) {
  impl_->headers.emplace_back(std::string(name), std::string(value));
}

std::string HttpResponseBuilder::ToString() const {
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

void HttpResponseBuilder::Html(std::move_only_function<void()> cb) {
  impl_->OpenTag("html", {});
  cb();
  impl_->CloseTag("html");
}

void HttpResponseBuilder::Head(std::move_only_function<void()> cb) {
  impl_->OpenTag("head", {});
  cb();
  impl_->CloseTag("head");
}

void HttpResponseBuilder::Body(std::move_only_function<void()> cb) {
  impl_->OpenTag("body", {});
  cb();
  impl_->CloseTag("body");
}

void HttpResponseBuilder::Header(std::move_only_function<void()> cb) {
  impl_->OpenTag("header", {});
  cb();
  impl_->CloseTag("header");
}

void HttpResponseBuilder::Div(std::move_only_function<void()> cb) {
  impl_->OpenTag("div", {});
  cb();
  impl_->CloseTag("div");
}

void HttpResponseBuilder::Div(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("div", a);
  cb();
  impl_->CloseTag("div");
}

void HttpResponseBuilder::H1(std::string_view t) {
  impl_->WriteLeaf("h1", t, {});
}
void HttpResponseBuilder::H2(std::string_view t) {
  impl_->WriteLeaf("h2", t, {});
}
void HttpResponseBuilder::H3(std::string_view t) {
  impl_->WriteLeaf("h3", t, {});
}

void HttpResponseBuilder::P(std::move_only_function<void()> cb) {
  impl_->OpenTag("p", {});
  cb();
  impl_->CloseTag("p");
}

void HttpResponseBuilder::Span(std::string_view t) {
  impl_->WriteLeaf("span", t, {});
}
void HttpResponseBuilder::Span(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("span", t, a);
}

void HttpResponseBuilder::Ul(std::move_only_function<void()> cb) {
  impl_->OpenTag("ul", {});
  cb();
  impl_->CloseTag("ul");
}

void HttpResponseBuilder::Li(std::string_view t) {
  impl_->WriteLeaf("li", t, {});
}

void HttpResponseBuilder::Table(std::move_only_function<void()> cb) {
  impl_->OpenTag("table", {});
  cb();
  impl_->CloseTag("table");
}

void HttpResponseBuilder::Table(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("table", a);
  cb();
  impl_->CloseTag("table");
}

void HttpResponseBuilder::Tr(std::move_only_function<void()> cb) {
  impl_->OpenTag("tr", {});
  cb();
  impl_->CloseTag("tr");
}

void HttpResponseBuilder::Th(std::string_view t) {
  impl_->WriteLeaf("th", t, {});
}
void HttpResponseBuilder::Td(std::string_view t) {
  impl_->WriteLeaf("td", t, {});
}
void HttpResponseBuilder::Td(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("td", t, a);
}

void HttpResponseBuilder::Form(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::move_only_function<void()> cb) {
  impl_->OpenTag("form", a);
  cb();
  impl_->CloseTag("form");
}

void HttpResponseBuilder::Label(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("label", t, a);
}

void HttpResponseBuilder::Input(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a) {
  impl_->WriteSelfClosing("input", a);
}

void HttpResponseBuilder::Button(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a,
    std::string_view t) {
  impl_->WriteLeaf("button", t, a);
}

void HttpResponseBuilder::Button(std::string_view t) {
  impl_->WriteLeaf("button", t, {});
}

void HttpResponseBuilder::Title(std::string_view t) {
  impl_->WriteLeaf("title", t, {});
}

void HttpResponseBuilder::Link(
    std::initializer_list<std::pair<std::string_view, std::string_view>> a) {
  impl_->WriteSelfClosing("link", a);
}

void HttpResponseBuilder::Text(std::string_view t) {
  impl_->WriteIndent();
  impl_->html_buffer.append(t);
  impl_->html_buffer.push_back('\n');
}

void HttpResponseBuilder::Raw(std::string_view html) {
  impl_->WriteIndent();
  impl_->html_buffer.append(html);
  impl_->html_buffer.push_back('\n');
}

}  // namespace http
