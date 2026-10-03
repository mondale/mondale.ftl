#ifndef HTTP_HTTP_SERVER_H_
#define HTTP_HTTP_SERVER_H_

#include "core/vocabulary.h"
#include "http/request_parser.h"
#include "io/epoller.h"
#include "net/listener.h"
#include "net/ssl_context.h"

namespace http {

class HttpServer final {
 private:
  struct PrivateTag {};

 public:
  explicit HttpServer(PrivateTag);

  static ResultOr<std::shared_ptr<HttpServer>> Create(uint16_t port);

  using HandlerFn = std::function<std::string(const RequestParser&)>;
  Result RegisterUriHandler(std::string_view uri, HandlerFn fn);

 private:
  void OnInboundConnection(core::FileDescriptor fd, net::SocketAddress sa);
  void Sniffed(bool is_tls, core::FileDescriptor fd, net::SocketAddress sa);
  Result FireUpHttp(core::FileDescriptor fd, net::SocketAddress sa);
  Result FireUpHttps(core::FileDescriptor fd, net::SocketAddress sa);
  Result Serve(Result r, const RequestParser& rp, io::Poster* p);

  std::unique_ptr<io::Epoller> epoller_;
  std::shared_ptr<net::Listener> listener_;
  std::unique_ptr<net::SslContext> ssl_context_;
  std::atomic<int64_t> generation_{0};

  Mutex mu_;
  std::unordered_map<std::string_view, HandlerFn> uri_map_ GUARDED_BY(mu_);

  struct Package {
    int64_t generation = 0;
    std::unordered_map<std::string_view, HandlerFn> uri_map;
  };

  Result Serve(const Package& package, const RequestParser& rp, io::Poster* p);
  void UpdateIfNeeded(Package* p) LOCKS_EXCLUDED(mu_);
  void Update(Package* p) LOCKS_EXCLUDED(mu_);
};

}  // namespace http

#endif  // #ifndef HTTP_HTTP_SERVER_H_
