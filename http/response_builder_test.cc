#include "http/response_builder.h"
#include "testing/testing.h"

namespace {

using http::ResponseBuilder;
using testing::HasSubstr;

class ResponseBuilderTest : public ::testing::Test {
 protected:
  std::string Render(std::move_only_function<void(ResponseBuilder&)> fn) {
    ResponseBuilder hrb;
    fn(hrb);
    return hrb.ToString();
  }
};

TEST_F(ResponseBuilderTest, RendersFullResponseWithHeaders) {
  std::string response = Render([](ResponseBuilder& hrb) {
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

TEST_F(ResponseBuilderTest, RendersAttributesAndInputs) {
  std::string response = Render([](ResponseBuilder& hrb) {
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

TEST_F(ResponseBuilderTest, RendersInjectedRawHtml) {
  std::string response = Render([](ResponseBuilder& hrb) {
    hrb.Div([&] { hrb.Raw("<div class=\"custom-widget\">Raw Snippet</div>"); });
  });

  EXPECT_THAT(
      response,
      HasSubstr(
          "<div>\n  <div class=\"custom-widget\">Raw Snippet</div>\n</div>"));
}

TEST_F(ResponseBuilderTest, Renders404NotFoundPage) {
  std::string response = Render([](ResponseBuilder& hrb) {
    hrb.SetStatus(404, "Not Found");
    hrb.SetHeader("Content-Type", "text/html; charset=UTF-8");
    hrb.SetHeader("Connection", "close");

    hrb.Doctype();
    hrb.Html({{"lang", "en"}}, [&] {
      hrb.Head([&] {
        hrb.Meta({{"charset", "UTF-8"}});
        hrb.Title("404 - Not Found");
        hrb.Style(
            "\n        body {\n"
            "            background-color: #1e1e1e;\n"
            "            color: #ffffff;\n"
            "            font-family: monospace;\n"
            "            display: flex;\n"
            "            flex-direction: column;\n"
            "            align-items: center;\n"
            "            justify-content: center;\n"
            "            height: 100vh;\n"
            "            margin: 0;\n"
            "        }\n"
            "        .container {\n"
            "            text-align: center;\n"
            "            padding: 2.5rem;\n"
            "            border: 1px solid #333333;\n"
            "            border-radius: 4px;\n"
            "            background-color: #252526;\n"
            "            max-width: 400px;\n"
            "        }\n"
            "        h1 {\n"
            "            font-size: 1.8rem;\n"
            "            margin-top: 0;\n"
            "            margin-bottom: 0.5rem;\n"
            "            color: #569cd6;\n"
            "        }\n"
            "        p {\n"
            "            color: #cccccc;\n"
            "            margin-bottom: 1.5rem;\n"
            "            line-height: 1.4;\n"
            "        }\n"
            "        a {\n"
            "            color: #569cd6;\n"
            "            text-decoration: none;\n"
            "            border-bottom: 1px dotted #569cd6;\n"
            "        }\n"
            "        a:hover {\n"
            "            color: #ffffff;\n"
            "            border-bottom: 1px solid #ffffff;\n"
            "        }\n"
            "    ");
      });
      hrb.Body([&] {
        hrb.Div({{"class", "container"}}, [&] {
          hrb.H1("404");
          hrb.P("The page you seek has drifted beyond the horizon.");
          hrb.A({{"href", "/"}}, "Return to Port");
        });
      });
    });
  });

  EXPECT_THAT(response, HasSubstr("HTTP/1.1 404 Not Found\r\n"));
  EXPECT_THAT(response,
              HasSubstr("Content-Type: text/html; charset=UTF-8\r\n"));
  EXPECT_THAT(response, HasSubstr("Connection: close\r\n"));
  EXPECT_THAT(response, HasSubstr("<!DOCTYPE html>\n"));
  EXPECT_THAT(response, HasSubstr("<html lang=\"en\">\n"));
  EXPECT_THAT(response, HasSubstr("<meta charset=\"UTF-8\" />\n"));
  EXPECT_THAT(response, HasSubstr("<style>"));
  EXPECT_THAT(response, HasSubstr("<div class=\"container\">\n"));
  EXPECT_THAT(response, HasSubstr("<h1>404</h1>\n"));
  EXPECT_THAT(
      response,
      HasSubstr("<p>The page you seek has drifted beyond the horizon.</p>\n"));
  EXPECT_THAT(response, HasSubstr("<a href=\"/\">Return to Port</a>"));
}

TEST_F(ResponseBuilderTest, RendersCompleteDashboardExample) {
  std::string response = Render([](ResponseBuilder& hrb) {
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

TEST_F(ResponseBuilderTest, RendersPreformattedBlock) {
  std::string response = Render([](ResponseBuilder& rb) {
    rb.Div([&] { rb.Pre("LOG_ENTRY: System initialized."); });
  });

  EXPECT_THAT(
      response,
      HasSubstr("<div>\n  <pre>LOG_ENTRY: System initialized.</pre>\n</div>"));
}

}  // namespace
