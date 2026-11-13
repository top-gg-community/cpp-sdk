#pragma once

#ifndef NGHTTP2_NO_SSIZE_T
#define NGHTTP2_NO_SSIZE_T
#endif

#include <nghttp2/nghttp2.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <topgg/topgg.h>
#include <uv.h>

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <thread>
#include <variant>

namespace topgg {
class http_frontend;
class http_request;

class http_backend {
  struct pending_write {
    std::vector<uint8_t> data{};
    size_t offset{};
  };

  http_frontend* m_frontend{};
  waker m_waker{};
  std::atomic<bool> m_shutdown{};
#ifdef _WIN32
  wsastartup_guard m_wsastartup_guard{};
#endif
  uv_loop_t* m_loop{nullptr};
  SSL_CTX* m_ssl_context{nullptr};
  SSL* m_ssl{nullptr};
  BIO* m_ssl_read_bio{nullptr};
  BIO* m_ssl_write_bio{nullptr};
  struct addrinfo* m_addrs{nullptr};
  uv_tcp_t m_socket{};
  uv_connect_t m_socket_connection{};
  bool m_shook_hand{};
  nghttp2_session* m_nghttp2{nullptr};
  std::deque<http_backend::pending_write> m_pending_writes{};
  std::vector<http_request*> m_ongoing_requests{};
  size_t m_open_requests{static_cast<size_t>(-1)};
  std::mutex m_flush_requests_mutex{};
  uv_async_t m_async_flush_requests{};
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  std::mutex m_oauth2_mutex{};
  uv_async_t m_async_flush_oauth2_requests{};
  std::vector<std::shared_ptr<oauth2_client>> m_oauth2_refresh_queue{};
  std::vector<std::shared_ptr<oauth2_client>> m_oauth2_clients{};
  std::vector<std::shared_ptr<oauth2_client>> m_oauth2_revoke_queue{};
#endif
  uv_async_t m_async_close{};
  std::optional<exception> m_error{std::nullopt};

  inline http_backend(http_frontend* frontend) : m_frontend(frontend) {}

  http_backend() = delete;

  inline void socket_throw(const exception& error) noexcept {
    m_error = error;

    shutdown();
  }

  void init();

  void connect();

  static void on_read(uv_stream_t* stream, ssize_t read_length,
                      const uv_buf_t* buf);

  static void on_connect(uv_connect_t* connection, int status);

  void flush();

  void flush_requests();

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  void flush_oauth2_requests();
#endif

  static nghttp2_ssize on_send(nghttp2_session* http2, const uint8_t* data,
                               size_t length, int flags, void* ptr);

  static int on_header(nghttp2_session* http2, const nghttp2_frame* frame,
                       const uint8_t* name, size_t name_length,
                       const uint8_t* value, size_t value_length, uint8_t flags,
                       void* ptr);

  static int on_data_chunk(nghttp2_session* http2, uint8_t flags,
                           int32_t stream_id, const uint8_t* data,
                           size_t length, void* ptr);

  void dispatch(http_request* request, uv_work_cb work_callback);

  static int on_stream_close(nghttp2_session* http2, int32_t stream_id,
                             uint32_t error, void* ptr);

  static void on_async_flush_requests(uv_async_t* handle);

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  static void on_async_flush_oauth2_requests(uv_async_t* handle);
#endif

  static void on_async_close(uv_async_t* handle);

  static void on_close(uv_handle_t* handle);

  void loop();

  void shutdown(const bool blocking = false);

  ~http_backend();

  friend class http_frontend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2;
  friend class oauth2_client;
#endif
};

struct http_response {
  uint16_t status{};
  std::optional<uint32_t> retry_after{std::nullopt};
  std::string_view body{};
};

using http_response_pair = std::variant<exception, http_response>;

class http_request {
  using callback = std::function<void(const http_response_pair&)>;

  std::vector<nghttp2_nv> m_headers{};
  std::string m_path{};
  std::string m_authorization{};
  std::string m_content_type{};
  std::string m_content_length{};
  std::string m_body{};
  size_t m_body_position{};
  http_request::callback m_callback{};
  uint16_t m_status{};
  std::optional<uint32_t> m_retry_after{};
  std::string m_response{};

  http_request(const std::string& token, const std::string_view& method,
               const std::string& path, const http_request::callback& callback,
               const std::string& body = "",
               const std::string& content_type = "application/json");

  void add_header(const std::string_view& name, const std::string_view& value);

  static ssize_t on_body_read(nghttp2_session* session, int32_t stream_id,
                              uint8_t* data, size_t length, uint32_t* flags,
                              nghttp2_data_source* source, void* ptr);

  inline void dispatch_response() {
    m_callback(
        http_response{m_status, m_retry_after, std::string_view{m_response}});
  }

  inline void dispatch_exception(const exception& error) { m_callback(error); }

 public:
  http_request() = delete;

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
  ~http_request();
#endif

  friend class base_client;
  friend class http_backend;
  friend class http_exception;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
  friend class oauth2_url;
  friend class oauth2;
#endif
};

class http_frontend {
  http_backend* m_backend{nullptr};
  std::thread m_backend_thread{};
  std::vector<http_request*> m_requests{};
  std::mutex m_requests_mutex{};

  void fetch(http_request* request, const bool defer = false);

 public:
  http_frontend();

  ~http_frontend();

  friend class base_client;
  friend class http_backend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2;
  friend class oauth2_url;
  friend class oauth2_client;
#endif
};
};  // namespace topgg