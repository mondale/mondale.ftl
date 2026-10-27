#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/process.h"
#include "core/file.h"
#include "core/file_descriptor.h"
#include "core/syscalls.h"
#include "core/vocabulary.h"
#include "http/canned_responses.h"
#include "http/handlers/logz.h"
#include "http/http_server.h"
#include "http/response_builder.h"

namespace http::handlers {
namespace {

constexpr size_t kMemTarget = 64 * 1024;     // 64 KiB nominal memory target
constexpr size_t kMemMax = 2 * 1024 * 1024;  // 2 MiB hard heap limit

enum class FilterMode { kShowAll, kFilterOnly, kSuppressOnly };

struct SeverityRule {
  char severity = 'I';
  FilterMode mode = FilterMode::kShowAll;
};

struct ThreadRule {
  std::string thread_id;
  FilterMode mode = FilterMode::kShowAll;
};

struct FilterParams {
  size_t page = 1;
  int64_t time_start_sec = 0;
  int64_t time_end_sec = 0;
  std::string time_start_raw;
  std::string time_end_raw;

  // Key: severity char ('I', 'W', 'E', 'F') -> mode
  std::unordered_map<char, std::string> severity_modes;
  // Key: thread ID string -> mode
  std::unordered_map<std::string, std::string> thread_modes;

  // Raw query params for URL generation
  std::unordered_map<std::string, std::string> raw_query;
};

struct LogEntry {
  char severity = 'I';
  std::string thread_id;
  std::string timestamp;
  std::string source_loc;
  std::string message;
  bool is_truncated = false;

  size_t ApproximateMemorySize() const {
    return sizeof(*this) + thread_id.capacity() + timestamp.capacity() +
           source_loc.capacity() + message.capacity();
  }
};

bool IsBrowserUserAgent(const RequestParser& rp) {
  return strings::StartsWith(rp.user_agent(), "Mozilla");
}

int64_t ParseDateTimeLocal(std::string_view s) {
  if (s.size() < 16) return 0;
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  if (std::sscanf(s.data(), "%d-%d-%dT%d:%d", &y, &m, &d, &h, &min) >= 5) {
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    struct tm tm = {};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = min;
    tm.tm_sec = 0;
    if (s.size() >= 19 && s[16] == ':') {
      int s_sec = 0;
      if (std::sscanf(s.data() + 17, "%d", &s_sec) == 1) {
        tm.tm_sec = s_sec;
      }
    }
    return static_cast<int64_t>(std::mktime(&tm));
  }
  return 0;
}

int64_t ParseLogTimestamp(std::string_view ts_str) {
  if (ts_str.size() < 19) return 0;
  int y = 0, m = 0, d = 0, h = 0, min = 0, s = 0;
  if (std::sscanf(ts_str.data(), "%d-%d-%d %d:%d:%d", &y, &m, &d, &h, &min,
                  &s) >= 6) {
    struct tm tm = {};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = min;
    tm.tm_sec = s;
    return static_cast<int64_t>(std::mktime(&tm));
  }
  return 0;
}

FilterParams ParseFilterParams(const RequestParser& rp) {
  FilterParams p;
  p.raw_query = rp.query_params();

  auto it_page = p.raw_query.find("p");
  if (it_page != p.raw_query.end()) {
    auto parsed = strings::ParseAs<int64_t>(it_page->second, 10);
    if (parsed.IsOk() && parsed.ValueOrDie() > 0) {
      p.page = static_cast<size_t>(parsed.ValueOrDie());
    }
  }

  auto it_tstart = p.raw_query.find("ts");
  if (it_tstart != p.raw_query.end()) {
    p.time_start_raw = it_tstart->second;
    p.time_start_sec = ParseDateTimeLocal(it_tstart->second);
  }

  auto it_tend = p.raw_query.find("te");
  if (it_tend != p.raw_query.end()) {
    p.time_end_raw = it_tend->second;
    p.time_end_sec = ParseDateTimeLocal(it_tend->second);
  }

  if (p.time_start_sec > 0 && p.time_end_sec > 0 &&
      p.time_start_sec > p.time_end_sec) {
    std::swap(p.time_start_sec, p.time_end_sec);
    std::swap(p.time_start_raw, p.time_end_raw);
  }

  for (const auto& [k, v] : p.raw_query) {
    if (strings::StartsWith(k, "sev_") && k.size() == 5) {
      char sev = k[4];
      p.severity_modes[sev] = v;
    } else if (strings::StartsWith(k, "th_")) {
      std::string th_id = k.substr(3);
      p.thread_modes[th_id] = v;
    }
  }

  return p;
}

bool IsLogEntryStart(std::string_view sv) {
  if (sv.empty()) return false;
  char s = sv[0];
  if (s != 'I' && s != 'W' && s != 'E' && s != 'F') return false;

  size_t i = 1;
  while (i < sv.size() && std::isdigit(static_cast<unsigned char>(sv[i]))) {
    ++i;
  }
  if (i == 1 || i >= sv.size()) return false;
  return sv[i] == ' ';
}

LogEntry ParseLogEntry(std::string_view line) {
  LogEntry entry;
  if (line.empty()) return entry;

  entry.severity = line[0];
  std::string_view rest = line.substr(1);

  size_t sp1 = rest.find(' ');
  if (sp1 == std::string_view::npos) {
    entry.message = std::string(line);
    return entry;
  }
  entry.thread_id = std::string(rest.substr(0, sp1));
  rest.remove_prefix(sp1 + 1);

  size_t sp2 = rest.find(' ');
  if (sp2 == std::string_view::npos) {
    entry.message = std::string(line);
    return entry;
  }
  size_t sp3 = rest.find(' ', sp2 + 1);
  if (sp3 == std::string_view::npos) {
    entry.message = std::string(line);
    return entry;
  }

  entry.timestamp = std::string(rest.substr(0, sp3));
  rest.remove_prefix(sp3 + 1);

  size_t bracket = rest.find("] ");
  if (bracket == std::string_view::npos) {
    entry.message = std::string(line);
    return entry;
  }

  entry.source_loc = std::string(rest.substr(0, bracket));
  entry.message = std::string(rest.substr(bracket + 2));

  return entry;
}

bool MatchesFilters(const LogEntry& entry, const FilterParams& params) {
  if (params.time_start_sec > 0 || params.time_end_sec > 0) {
    int64_t entry_sec = ParseLogTimestamp(entry.timestamp);
    if (entry_sec > 0) {
      if (params.time_start_sec > 0 && entry_sec < params.time_start_sec) {
        return false;
      }
      if (params.time_end_sec > 0 && entry_sec > params.time_end_sec) {
        return false;
      }
    }
  }

  bool has_sev_filter = false;
  bool sev_matched = false;

  for (const auto& [sev, mode] : params.severity_modes) {
    if (mode == "suppress" && entry.severity == sev) {
      return false;
    }
    if (mode == "filter") {
      has_sev_filter = true;
      if (entry.severity == sev) {
        sev_matched = true;
      }
    }
  }
  if (has_sev_filter && !sev_matched) {
    return false;
  }

  bool has_th_filter = false;
  bool th_matched = false;

  for (const auto& [th, mode] : params.thread_modes) {
    if (mode == "suppress" && entry.thread_id == th) {
      return false;
    }
    if (mode == "filter") {
      has_th_filter = true;
      if (entry.thread_id == th) {
        th_matched = true;
      }
    }
  }
  if (has_th_filter && !th_matched) {
    return false;
  }

  return true;
}

std::vector<LogEntry> ReadLogPagesBackward(const core::FileDescriptor& fd,
                                           off_t file_size,
                                           const FilterParams& params,
                                           size_t* total_pages) {
  std::vector<LogEntry> page_entries;
  if (file_size <= 0) {
    *total_pages = 1;
    return page_entries;
  }

  size_t current_heap_bytes = 0;
  size_t current_page = 1;

  ResultOr<std::string> file_contents =
      core::ReadContentsFromFile(base::internal::GetLogPath());
  if (!file_contents.IsOk()) {
    *total_pages = 1;
    return page_entries;
  }

  std::string_view content = file_contents.ValueOrDie();
  size_t end_idx = content.size();
  while (end_idx > 0) {
    size_t newline_pos = content.rfind('\n', end_idx - 1);
    size_t line_start =
        (newline_pos == std::string_view::npos) ? 0 : newline_pos + 1;
    std::string_view line = content.substr(line_start, end_idx - line_start);

    if (IsLogEntryStart(line)) {
      LogEntry entry = ParseLogEntry(line);
      if (entry.ApproximateMemorySize() > kMemMax / 2) {
        entry.message = entry.message.substr(0, kMemMax / 2) +
                        "\n... [log file too long for memory limits]";
        entry.is_truncated = true;
      }

      if (MatchesFilters(entry, params)) {
        size_t entry_bytes = entry.ApproximateMemorySize();
        if (current_heap_bytes + entry_bytes > kMemTarget &&
            !page_entries.empty()) {
          current_page++;
          current_heap_bytes = 0;
        }

        if (current_page == params.page) {
          page_entries.push_back(entry);
        }
        current_heap_bytes += entry_bytes;
      }
    }

    if (newline_pos == std::string_view::npos) break;
    end_idx = newline_pos;
  }

  *total_pages = std::max(size_t{1}, current_page);
  return page_entries;
}

std::string BuildQueryUrl(
    const FilterParams& params, size_t target_page,
    const std::vector<std::pair<std::string, std::string>>& overrides = {}) {
  std::unordered_map<std::string, std::string> q = params.raw_query;
  q["p"] = std::to_string(target_page);

  for (const auto& [k, v] : overrides) {
    if (v.empty()) {
      q.erase(k);
    } else {
      q[k] = v;
    }
  }

  std::string url = "/logz";
  bool first = true;
  for (const auto& [k, v] : q) {
    url.push_back(first ? '?' : '&');
    first = false;
    url.append(k);
    url.push_back('=');
    url.append(v);
  }
  return url;
}

constexpr char kCustomLogzStyle[] = R"(
  body {
    background-color: #121212;
    color: #e0e0e0;
    font-family: 'Cascadia Code', 'Fira Code', Consolas, monospace;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    height: 100vh;
  }
  .topbar {
    position: sticky;
    top: 0;
    background-color: #1e1e1e;
    border-bottom: 1px solid #333333;
    padding: 0.75rem 1rem;
    z-index: 100;
    display: flex;
    flex-wrap: wrap;
    gap: 1rem;
    align-items: center;
  }
  .topbar-section {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }
  .topbar-label {
    font-weight: bold;
    color: #569cd6;
    font-size: 0.85rem;
  }
  .btn {
    background-color: #2d2d2d;
    color: #cccccc;
    border: 1px solid #444444;
    padding: 0.2rem 0.5rem;
    border-radius: 3px;
    text-decoration: none;
    font-size: 0.8rem;
  }
  .btn:hover {
    background-color: #3e3e3e;
    color: #ffffff;
  }
  .btn-active {
    background-color: #0055ff;
    color: #ffffff;
    border-color: #569cd6;
  }
  .log-container {
    flex: 1;
    overflow-y: auto;
    padding: 1rem;
  }
  .log-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.85rem;
  }
  .log-row {
    border-bottom: 1px solid #222222;
  }
  .log-row-highlight {
    background-color: #1a2733;
  }
  .log-cell {
    padding: 0.25rem 0.75rem;
    vertical-align: top;
  }
  .log-cell-meta {
    white-space: nowrap;
  }
  .log-cell-msg {
    white-space: pre-wrap;
    word-break: break-word;
    width: 100%;
  }
  .sev-I { color: #569cd6; font-weight: bold; }
  .sev-W { color: #ce9178; font-weight: bold; }
  .sev-E { color: #f44747; font-weight: bold; }
  .sev-F { color: #ff0000; background-color: #400000; font-weight: bold; }
)";

ResultOr<std::string> RenderHtmlView(const RequestParser& rp,
                                     const std::vector<LogEntry>& entries,
                                     size_t current_page, size_t total_pages,
                                     const FilterParams& params) {
  ResponseBuilder rb;
  rb.SetStatus(200, "OK");
  rb.SetHeader("Content-Type", "text/html; charset=utf-8");
  rb.RespondingTo(rp);

  rb.Doctype();
  rb.Html({{"lang", "en"}}, [&] {
    rb.Head([&] {
      rb.Meta({{"charset", "UTF-8"}});
      rb.Title("logz viewer");
      rb.Style(kCustomLogzStyle);
    });

    rb.Body([&] {
      rb.Div({{"class", "topbar"}}, [&] {
        rb.Div({{"class", "topbar-section"}}, [&] {
          rb.Span({{"class", "topbar-label"}}, "Pages:");
          if (current_page > 1) {
            rb.A({{"class", "btn"},
                  {"href", BuildQueryUrl(params, current_page - 1)}},
                 "Prev");
          }
          for (size_t p = 1; p <= total_pages; ++p) {
            std::string cls = (p == current_page) ? "btn btn-active" : "btn";
            rb.A({{"class", cls}, {"href", BuildQueryUrl(params, p)}},
                 std::to_string(p));
          }
          if (current_page < total_pages) {
            rb.A({{"class", "btn"},
                  {"href", BuildQueryUrl(params, current_page + 1)}},
                 "Next");
          }
        });

        rb.Div({{"class", "topbar-section"}}, [&] {
          rb.Span({{"class", "topbar-label"}}, "Time:");
          rb.A({{"class", "btn"},
                {"href", BuildQueryUrl(params, 1, {{"ts", ""}, {"te", ""}})}},
               "All");
          int64_t now_sec =
              base::WallTime::Now().UnixNanoseconds() / 1'000'000'000;
          rb.A({{"class", "btn"},
                {"href",
                 BuildQueryUrl(params, 1,
                               {{"ts", std::to_string(now_sec - 3600)}})}},
               "1h");
          rb.A({{"class", "btn"},
                {"href",
                 BuildQueryUrl(params, 1,
                               {{"ts", std::to_string(now_sec - 86400)}})}},
               "24h");

          rb.Form(
              {{"method", "GET"},
               {"action", "/logz"},
               {"style",
                "display:inline; display:flex; gap:0.3rem; align-items:center; "
                "margin:0;"}},
              [&] {
                for (const auto& [k, v] : params.raw_query) {
                  if (k != "ts" && k != "te" && k != "p") {
                    rb.Input({{"type", "hidden"}, {"name", k}, {"value", v}});
                  }
                }
                rb.Input({{"type", "datetime-local"},
                          {"name", "ts"},
                          {"value", params.time_start_raw},
                          {"class", "btn"},
                          {"style", "font-family:inherit;"}});
                rb.Span("to");
                rb.Input({{"type", "datetime-local"},
                          {"name", "te"},
                          {"value", params.time_end_raw},
                          {"class", "btn"},
                          {"style", "font-family:inherit;"}});
                rb.Button({{"class", "btn"}, {"type", "submit"}}, "Filter");
              });
        });

        rb.Div({{"class", "topbar-section"}}, [&] {
          rb.Span({{"class", "topbar-label"}}, "Severities:");
          for (char s : {'I', 'W', 'E', 'F'}) {
            std::string key = std::string("sev_") + s;
            auto it = params.severity_modes.find(s);
            std::string cur_mode =
                (it != params.severity_modes.end()) ? it->second : "show";

            std::string next_mode = "show";
            if (cur_mode == "show")
              next_mode = "highlight";
            else if (cur_mode == "highlight")
              next_mode = "filter";
            else if (cur_mode == "filter")
              next_mode = "suppress";

            std::string btn_cls = "btn";
            if (cur_mode == "highlight" || cur_mode == "filter")
              btn_cls += " btn-active";

            std::string lbl = std::string(1, s) + " (" + cur_mode + ")";
            rb.A({{"class", btn_cls},
                  {"href", BuildQueryUrl(params, 1, {{key, next_mode}})}},
                 lbl);
          }
        });
      });

      rb.Div({{"class", "log-container"}}, [&] {
        rb.Table({{"class", "log-table"}}, [&] {
          for (const auto& entry : entries) {
            auto it_sev = params.severity_modes.find(entry.severity);
            bool highlight = (it_sev != params.severity_modes.end() &&
                              it_sev->second == "highlight");

            auto it_th = params.thread_modes.find(entry.thread_id);
            if (it_th != params.thread_modes.end() &&
                it_th->second == "highlight") {
              highlight = true;
            }

            std::string row_cls =
                highlight ? "log-row log-row-highlight" : "log-row";
            rb.Tr({{"class", row_cls}}, [&] {
              std::string sev_cls =
                  std::string("log-cell log-cell-meta sev-") + entry.severity;
              rb.Td({{"class", sev_cls}}, std::string(1, entry.severity));
              rb.Td({{"class", "log-cell log-cell-meta"}}, entry.thread_id);
              rb.Td({{"class", "log-cell log-cell-meta"}}, entry.timestamp);
              rb.Td({{"class", "log-cell log-cell-meta"}}, entry.source_loc);
              rb.Td({{"class", "log-cell log-cell-msg"}}, entry.message);
            });
          }
        });
      });
    });
  });

  return rb.ToString();
}

ResultOr<std::string> RenderPlainText(const RequestParser& rp,
                                      const std::vector<LogEntry>& entries) {
  ResponseBuilder rb;
  rb.SetStatus(200, "OK");
  rb.SetHeader("Content-Type", "text/plain; charset=utf-8");
  rb.RespondingTo(rp);

  std::string text_body;
  for (const auto& e : entries) {
    text_body.push_back(e.severity);
    text_body.append(e.thread_id);
    text_body.push_back(' ');
    text_body.append(e.timestamp);
    text_body.push_back(' ');
    text_body.append(e.source_loc);
    text_body.append("] ");
    text_body.append(e.message);
    text_body.push_back('\n');
  }

  rb.Raw(text_body);
  return rb.ToString();
}

}  // namespace

ResultOr<std::string> ServeLogz(const RequestParser& rp) {
  base::FlushLogs();

  std::string log_path = base::internal::GetLogPath();
  ResultOr<core::FileDescriptor> fd_res =
      core::syscalls::Open(log_path, O_RDONLY, 0);
  if (!fd_res.IsOk()) {
    ResponseBuilder rb;
    CannedResponses::Instance()->NotFound404(
        rb, strings::Format("Log file [{}] unavailable", log_path));
    return rb.ToString();
  }
  const core::FileDescriptor& fd = fd_res.ValueOrDie();

  ResultOr<struct stat> stat_res = core::syscalls::FStat(fd);
  if (!stat_res.IsOk()) {
    ResponseBuilder rb;
    CannedResponses::Instance()->NotFound404(rb, "Failed to stat log file");
    return rb.ToString();
  }

  FilterParams params = ParseFilterParams(rp);
  size_t total_pages = 1;
  std::vector<LogEntry> entries = ReadLogPagesBackward(
      fd, stat_res.ValueOrDie().st_size, params, &total_pages);

  if (IsBrowserUserAgent(rp)) {
    return RenderHtmlView(rp, entries, params.page, total_pages, params);
  }

  return RenderPlainText(rp, entries);
}

static bool reg_logz = RegisterGlobalUriHandler(
    "/logz", [](const RequestParser& rp) { return ServeLogz(rp); });

}  // namespace http::handlers
