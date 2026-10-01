#include "http/response_builder.h"
#include "testing/testing.h"

namespace {

using http::HttpResponseBuilder;
using testing::HasSubstr;

class HttpResponseBuilderTest : public ::testing::Test {
 protected:
  std::string Render(std::move_only_function<void(HttpResponseBuilder&)> fn) {
    HttpResponseBuilder hrb;
    fn(hrb);
    return hrb.ToString();
  }
};

TEST_F(HttpResponseBuilderTest, RendersFullHttpResponseWithHeaders) {
  std::string response = Render([](HttpResponseBuilder& hrb) {
    hrb.SetStatus(200, "OK");
    hrb.SetHeader("Content-Type", "text/html; charset=utf-8");
    hrb.Html([&] {
      hrb.Head([&] { hrb.Title("Status Dashboard"); });
      hrb.Body([&] { hrb.H1("Node Control Panel"); });
    });
  });

  EXPECT_THAT(response, HasSubstr("HTTP/1.1 200 OK\r\n"));
  EXPECT_THAT(response,
              HasSubstr("Content-Type: text/html; charset=utf-8\r\n"));
  EXPECT_THAT(response, HasSubstr("Content-Length: "));
  EXPECT_THAT(response, HasSubstr("\r\n\r\n<html>\n"));
  EXPECT_THAT(response, HasSubstr("<h1>Node Control Panel</h1>"));
}

TEST_F(HttpResponseBuilderTest, RendersAttributesAndInputs) {
  std::string response = Render([](HttpResponseBuilder& hrb) {
    hrb.Div({{"class", "card"}, {"id", "panel-1"}}, [&] {
      hrb.Form({{"method", "POST"}, {"action", "/apply"}}, [&] {
        hrb.Input({{"type", "number"}, {"name", "timeout"}, {"value", "500"}});
        hrb.Button({{"type", "submit"}, {"class", "btn"}}, "Save");
      });
    });
  });

  EXPECT_THAT(response, HasSubstr("<div class=\"card\" id=\"panel-1\">"));
  EXPECT_THAT(response, HasSubstr("<form method=\"POST\" action=\"/apply\">"));
  EXPECT_THAT(
      response,
      HasSubstr("<input type=\"number\" name=\"timeout\" value=\"500\" />"));
  EXPECT_THAT(response,
              HasSubstr("<button type=\"submit\" class=\"btn\">Save</button>"));
}

TEST_F(HttpResponseBuilderTest, RendersInjectedRawHtml) {
  std::string response = Render([](HttpResponseBuilder& hrb) {
    hrb.Div([&] { hrb.Raw("<div class=\"custom-widget\">Raw Snippet</div>"); });
  });

  EXPECT_THAT(
      response,
      HasSubstr(
          "<div>\n  <div class=\"custom-widget\">Raw Snippet</div>\n</div>"));
}

TEST_F(HttpResponseBuilderTest, RendersCompleteDashboardExample) {
  std::string response = Render([](HttpResponseBuilder& hrb) {
    hrb.SetStatus(200, "OK");
    hrb.SetHeader("Content-Type", "text/html; charset=utf-8");
    hrb.Html([&] {
      hrb.Head([&] {
        hrb.Title("Node Control Panel");
        hrb.Link({{"rel", "stylesheet"}, {"href", "/static/main.css"}});
      });
      hrb.Body([&] {
        hrb.Header([&] {
          hrb.H1("Cluster Status");
          hrb.P([&] {
            hrb.Text("Logged in as ");
            hrb.Span({{"class", "username"}}, "admin");
          });
        });
        hrb.Div({{"class", "card-container"}}, [&] {
          hrb.Table({{"class", "metrics-table"}}, [&] {
            hrb.Tr([&] {
              hrb.Th("Worker ID");
              hrb.Th("Status");
            });
            hrb.Tr([&] {
              hrb.Td("worker-01");
              hrb.Td({{"class", "ok"}}, "Healthy");
            });
          });
        });
      });
    });
  });

  constexpr std::string_view kExpectedResponse =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "Content-Length: 560\r\n\r\n"
      "<html>\n"
      "  <head>\n"
      "    <title>Node Control Panel</title>\n"
      "    <link rel=\"stylesheet\" href=\"/static/main.css\" />\n"
      "  </head>\n"
      "  <body>\n"
      "    <header>\n"
      "      <h1>Cluster Status</h1>\n"
      "      <p>\n"
      "        Logged in as \n"
      "        <span class=\"username\">admin</span>\n"
      "      </p>\n"
      "    </header>\n"
      "    <div class=\"card-container\">\n"
      "      <table class=\"metrics-table\">\n"
      "        <tr>\n"
      "          <th>Worker ID</th>\n"
      "          <th>Status</th>\n"
      "        </tr>\n"
      "        <tr>\n"
      "          <td>worker-01</td>\n"
      "          <td class=\"ok\">Healthy</td>\n"
      "        </tr>\n"
      "      </table>\n"
      "    </div>\n"
      "  </body>\n"
      "</html>\n";

  EXPECT_EQ(response, kExpectedResponse);
}

}  // namespace
