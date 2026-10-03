#include "http/http_server.h"
#include "http/http_stream.h"
#include "http/web_socket.h"
#include "net/ssl_handshaker.h"
#include "net/ssl_socket.h"
#include "net/tls_sniffer.h"

namespace http {

HttpServer::HttpServer(PrivateTag) {}

// static
ResultOr<std::shared_ptr<HttpServer>> HttpServer::Create(uint16_t port) {
  auto ret = std::make_shared<HttpServer>(PrivateTag{});
  auto* const r = ret.get();

  // Fire up SSL.
  TRY_ASSIGN(ret->ssl_context_, net::SslContext::BuildServer());

  // Fire up an Epoller.
  TRY_ASSIGN(ret->epoller_, io::Epoller::Build(1));
  auto* const e = ret->epoller_.get();

  // Fire up a listen socket.
  TRY_ASSIGN(ret->listener_,
             net::Listener::Build(
                 e, port, [&](core::FileDescriptor fd, net::SocketAddress sa) {
                   r->OnInboundConnection(std::move(fd), sa);
                 }));
  return ret;
}

void HttpServer::OnInboundConnection(core::FileDescriptor fd,
                                     net::SocketAddress sa) {
  auto sniffer = net::TlsSniffer::New(
      sa, [this](bool is_tls, core::FileDescriptor fd, net::SocketAddress sa) {
        Sniffed(is_tls, std::move(fd), sa);
      });
  CHECK_OK(
      epoller_->SetNonBlockingAndRegister(std::move(sniffer), std::move(fd)));
}

void HttpServer::Sniffed(bool is_tls, core::FileDescriptor fd,
                         net::SocketAddress sa) {
  Result r;
  if (is_tls) {
    Log(INFO) << "Inbound HTTPS/TLS from [" << sa.ToString() << "]";
    r = FireUpHttps(std::move(fd), sa);
  } else {
    Log(INFO) << "Inbound HTTP from [" << sa.ToString() << "]";
    r = FireUpHttp(std::move(fd), sa);
  }
  if (r.IsOk()) return;
  Log(ERROR) << "Failed to establish connection from [" << sa.ToString()
             << "]: " << r;
}

Result HttpServer::FireUpHttp(core::FileDescriptor fd, net::SocketAddress sa) {
  TRY_ASSIGN(auto ws,
             WebSocket::New(epoller_.get(), std::move(fd), std::move(sa)));
  auto stream = std::make_unique<HttpStream>(
      ws.get(),
      [this](Result r, const RequestParser& rp, io::Poster* p) -> Result {
        return Serve(r, rp, p);
      });
  auto* const ptr = stream.get();
  ws->Encumber([s = std::move(stream)]() {});
  return epoller_->RunWithAffinity(ws, [p = ptr] { p->Prime(); });
}

Result HttpServer::FireUpHttps(core::FileDescriptor fd, net::SocketAddress sa) {
  TRY_ASSIGN(auto ssls, net::SslSocket::New(epoller_.get(), ssl_context_.get(),
                                            std::move(fd), std::move(sa),
                                            net::SslHandshaker{}));
  auto stream = std::make_unique<HttpStream>(
      ssls.get(),
      [this](Result r, const RequestParser& rp, io::Poster* p) -> Result {
        return Serve(r, rp, p);
      });
  auto* const ptr = stream.get();
  ssls->Encumber([s = std::move(stream)]() {});
  return epoller_->RunWithAffinity(ssls, [p = ptr] { p->Prime(); });
}

Result HttpServer::Serve(Result r, const RequestParser& rp, io::Poster* p) {
  Log(ERROR, If(!r.IsOk())) << r;
  thread_local static Package* package = new Package;
  UpdateIfNeeded(package);
  return Serve(*package, rp, p);
}

Result HttpServer::Serve(const Package& package, const RequestParser& rp,
                         io::Poster* p) {
  //
  constexpr char k404[] =
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content-Length: 9\r\n"
      "Connection: close\r\n"
      "\r\n"
      "Not Found";
  p->Post(io::SourceBuffer(k404, sizeof(k404), []() {}));
  p->PostClose();
  return Result::Ok();
}

Result HttpServer::RegisterUriHandler(std::string_view uri, HandlerFn fn) {
  MutexLock l(&mu_);
  if (uri_map_.find(uri) != uri_map_.end()) {
    return core::PreconditionError(
        strings::Format("Extant handler for URI [{}].", uri));
  }

  uri_map_.insert(std::make_pair(uri, fn));
  generation_.store(generation_.load(std::memory_order_relaxed) + 1,
                    std::memory_order_relaxed);

  return Result::Ok();
}

void HttpServer::UpdateIfNeeded(Package* p) {
  if (p->generation == generation_.load(std::memory_order_relaxed)) {
    return;
  }
  Update(p);
}

void HttpServer::Update(Package* p) {
  if (!mu_.TryLock()) return;
  p->uri_map = uri_map_;
  p->generation = generation_.load(std::memory_order_acquire);
  mu_.Unlock();
}

}  // namespace http
