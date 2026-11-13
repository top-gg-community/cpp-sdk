#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <openssl/err.h>
#include <topgg/topgg.h>

#include <algorithm>
#include <cstring>
#include <vector>

#if (defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)) && \
    defined(TOPGG_SSLKEYLOG)
#include <cstdio>
#endif

void topgg::http_backend::init() {
  m_socket.data = nullptr;
  m_async_flush_requests.data = nullptr;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  m_async_flush_oauth2_requests.data = nullptr;
#endif
  m_async_close.data = nullptr;
  m_socket_connection.data = this;

  if ((m_loop = uv_default_loop()) == nullptr) {
    throw topgg::exception{"Unable to retrieve default backend loop"};
  }

#ifdef _WIN32
  m_wsastartup_guard.init();
#endif

  SSL_library_init();
  SSL_load_error_strings();

  if ((m_ssl_context = SSL_CTX_new(TLS_client_method())) == nullptr) {
    throw topgg::exception::ssl("Unable to create SSL context");
  }

  SSL_CTX_set_options(m_ssl_context,
                      SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                          SSL_OP_NO_COMPRESSION |
                          SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

  if (SSL_CTX_set_alpn_protos(m_ssl_context, (const uint8_t*)"\x02h2", 3) !=
      0) {
    throw topgg::exception::ssl(
        "Unable to configure SSL context to use HTTP/2");
  }
#ifdef _WIN32
  else if (SSL_CTX_load_verify_store(m_ssl_context, "org.openssl.winstore:") ==
           0) {
    throw topgg::exception::ssl(
        "Unable to configure SSL context to use Windows System Certificate "
        "Store");
  }
#endif

#if (defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)) && \
    defined(TOPGG_SSLKEYLOG)
  SSL_CTX_set_keylog_callback(m_ssl_context,
                              [](const SSL* ssl, const char* line) {
                                auto keylog{fopen(TOPGG_SSLKEYLOG, "a")};

                                if (keylog != nullptr) {
                                  fprintf(keylog, "%s\n", line);
                                  fclose(keylog);
                                }
                              });
#endif

  SSL_CTX_set_verify(m_ssl_context, SSL_VERIFY_PEER, nullptr);

  if (SSL_CTX_set_default_verify_paths(m_ssl_context) == 0) {
    throw topgg::exception::ssl(
        "Unable to configure SSL context to perform certificate verification");
  }

  struct addrinfo hints{};

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo("top.gg", "443", &hints, &m_addrs) < 0 ||
      m_addrs == nullptr) {
    throw topgg::exception{"Unable to retrieve top.gg's IP address"};
  }
}

void topgg::http_backend::connect() {
  TOPGG_LOG("Connecting to Top.gg");

  if (const auto status{uv_tcp_init(m_loop, &m_socket)}; status < 0) {
    socket_throw(topgg::exception::uv(
        "Unable to create UV async oauth2 refresh", status));
  } else {
    m_socket.data = this;

    uv_tcp_connect(&m_socket_connection, &m_socket, m_addrs->ai_addr,
                   topgg::http_backend::on_connect);
  }
}

void topgg::http_backend::on_read(uv_stream_t* stream, ssize_t read_length,
                                  const uv_buf_t* buf) {
  TOPGG_LOGF("[EVENT: UV TCP READ] %d bytes", read_length);

  const auto self{reinterpret_cast<topgg::http_backend*>(stream->data)};

  if (read_length < 0) {
    if (read_length == UV_EOF) {
      self->shutdown();
    } else {
      self->socket_throw(
          topgg::exception::uv("Unable to continue reading from socket stream",
                               static_cast<int>(read_length)));
    }
  } else if (read_length > 0) {
    if (const auto status{BIO_write(self->m_ssl_read_bio, buf->base,
                                    static_cast<int>(read_length))};
        status != read_length) {
      self->socket_throw(topgg::exception::ssl(
          "Unable to write entire socket stream data to BIO"));
    } else {
#ifndef TOPGG_PROJECT_TOKENS_ONLY
      self->flush_oauth2_requests();
#endif
      self->flush_requests();
    }
  }

  delete[] buf->base;
}

void topgg::http_backend::on_connect(uv_connect_t* connection, int status) {
  TOPGG_LOGF("[EVENT: UV TCP CONNECT] status: %d", status);

  const auto self{reinterpret_cast<topgg::http_backend*>(connection->data)};

  self->m_open_requests = static_cast<size_t>(-1);

  if (status < 0) {
    return self->socket_throw(topgg::exception::uv(
        "Unable to perform TCP handshake with Top.gg", status));
  } else if ((self->m_ssl_read_bio = BIO_new(BIO_s_mem())) == nullptr ||
             (self->m_ssl_write_bio = BIO_new(BIO_s_mem())) == nullptr) {
    return self->socket_throw(topgg::exception::ssl("Unable to create BIO"));
  } else if ((self->m_ssl = SSL_new(self->m_ssl_context)) == nullptr ||
             SSL_set_tlsext_host_name(self->m_ssl, "top.gg") == 0 ||
             X509_VERIFY_PARAM_set1_host(SSL_get0_param(self->m_ssl), "top.gg",
                                         0) == 0) {
    return self->socket_throw(
        topgg::exception::ssl("Unable to create and configure SSL instance"));
  }

  self->m_flush_requests_mutex.lock();

  if ((status = uv_async_init(self->m_loop, &self->m_async_flush_requests,
                              topgg::http_backend::on_async_flush_requests)) <
      0) {
    self->m_flush_requests_mutex.unlock();

    return self->socket_throw(topgg::exception::uv(
        "Unable to create UV async flush requests", status));
  }

  self->m_async_flush_requests.data = self;
  self->m_flush_requests_mutex.unlock();

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  self->m_oauth2_mutex.lock();

  if ((status = uv_async_init(
           self->m_loop, &self->m_async_flush_oauth2_requests,
           topgg::http_backend::on_async_flush_oauth2_requests)) < 0) {
    self->m_oauth2_mutex.unlock();

    return self->socket_throw(topgg::exception::uv(
        "Unable to create UV async flush oauth2 requests", status));
  }

  self->m_async_flush_oauth2_requests.data = self;
  self->m_oauth2_mutex.unlock();
#endif

  if ((status = uv_async_init(self->m_loop, &self->m_async_close,
                              topgg::http_backend::on_async_close)) < 0) {
    return self->socket_throw(
        topgg::exception::uv("Unable to create UV async close", status));
  }

  self->m_async_close.data = self;

  SSL_set_bio(self->m_ssl, self->m_ssl_read_bio, self->m_ssl_write_bio);
  SSL_set_connect_state(self->m_ssl);

  BIO_set_conn_hostname(self->m_ssl_read_bio, "top.gg:443");
  BIO_set_conn_hostname(self->m_ssl_write_bio, "top.gg:443");

  if ((status = uv_read_start(
           reinterpret_cast<uv_stream_t*>(&self->m_socket),
           []([[maybe_unused]] uv_handle_t* handle, size_t length,
              uv_buf_t* buf) {
             buf->base = new char[length];
             buf->len = static_cast<unsigned long>(length);
           },
           topgg::http_backend::on_read)) < 0) {
    return self->socket_throw(
        topgg::exception::uv("Unable to start reading socket stream", status));
  }

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  self->flush_oauth2_requests();
#endif
  self->flush_requests();
}

void topgg::http_backend::flush() {
  TOPGG_LOG("Flushing pending I/O bytes");

  uint8_t buf[8 * 1024];

  while (BIO_pending(m_ssl_write_bio) > 0) {
    const auto read_length{BIO_read(m_ssl_write_bio, buf, sizeof(buf))};

    if (read_length <= 0) {
      break;
    }

    auto* raw_uv_buf{new std::vector<uint8_t>(buf, buf + read_length)};
    auto request{new uv_write_t};

    request->data = raw_uv_buf;

    uv_buf_t uv_buf{};

    uv_buf.base = reinterpret_cast<char*>(raw_uv_buf->data());
    uv_buf.len = static_cast<unsigned long>(raw_uv_buf->size());

    const auto status{uv_write(
        request, reinterpret_cast<uv_stream_t*>(&m_socket), &uv_buf, 1,
        [](uv_write_t* request, [[maybe_unused]] int status) {
          delete reinterpret_cast<std::vector<uint8_t>*>(request->data);
          delete request;
        })};

    if (status < 0) {
      delete raw_uv_buf;
      delete request;
      return socket_throw(topgg::exception::uv(
          "Unable to perform SSL handshake with Top.gg", status));
    }
  }
}

void topgg::http_backend::flush_requests() {
  if (!m_shook_hand) {
    if (const auto handshake_status{SSL_do_handshake(m_ssl)};
        handshake_status == 1) {
      TOPGG_LOG("Handshake successful");

      auto status{SSL_get_verify_result(m_ssl)};

      if (status != X509_V_OK) {
        return socket_throw("Unable to verify certificate");
      }

      const uint8_t* alpn{nullptr};
      unsigned int alpn_length{};

      SSL_get0_alpn_selected(m_ssl, &alpn, &alpn_length);

      if (alpn == nullptr || alpn_length != 2 || memcmp("h2", alpn, 2) != 0) {
        return socket_throw("Unable to negotiate HTTP/2");
      } else if ((status = uv_tcp_nodelay(&m_socket, 1)) < 0) {
        return socket_throw(topgg::exception::uv(
            "Unable to configure TCP file descriptor to no delay", status));
      }

      nghttp2_session_callbacks* callbacks{nullptr};

      if (nghttp2_session_callbacks_new(&callbacks) < 0) {
        return socket_throw("Out of memory");
      }

      nghttp2_session_callbacks_set_send_callback2(
          callbacks, topgg::http_backend::on_send);
      nghttp2_session_callbacks_set_on_header_callback(
          callbacks, topgg::http_backend::on_header);
      nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
          callbacks, topgg::http_backend::on_data_chunk);
      nghttp2_session_callbacks_set_on_stream_close_callback(
          callbacks, topgg::http_backend::on_stream_close);

      status = nghttp2_session_client_new(&m_nghttp2, callbacks, this);
      nghttp2_session_callbacks_del(callbacks);

      if (status < 0) {
        return socket_throw("Out of memory");
      }

      nghttp2_settings_entry settings[] = {
          {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

      if ((status = nghttp2_submit_settings(m_nghttp2, NGHTTP2_FLAG_NONE,
                                            settings, 1)) < 0) {
        return socket_throw(topgg::exception::nghttp2(
            "Unable to submit HTTP request settings", status));
      }

      m_shook_hand = true;
      m_open_requests = 0;
    } else {
      if (const auto handshake_error{SSL_get_error(m_ssl, handshake_status)};
          handshake_error != SSL_ERROR_WANT_READ &&
          handshake_error != SSL_ERROR_WANT_WRITE) {
        return socket_throw(topgg::exception::ssl(
            "Unable to perform SSL handshake with Top.gg"));
      }

      TOPGG_LOG("Flushing handshake");

      return flush();
    }
  }

  TOPGG_LOG("Flushing pending requests");

  int status{};

  m_frontend->m_requests_mutex.lock();

  while (!m_frontend->m_requests.empty()) {
    const auto request{m_frontend->m_requests.back()};

    m_frontend->m_requests.pop_back();
    m_frontend->m_requests_mutex.unlock();

    m_open_requests++;

    nghttp2_data_provider2 body_provider{};

    body_provider.source.ptr = request;
    body_provider.read_callback = topgg::http_request::on_body_read;

    TOPGG_LOGF("Submitting request to %s", request->m_path.c_str());

    if ((status = nghttp2_submit_request2(
             m_nghttp2, nullptr, request->m_headers.data(),
             request->m_headers.size(),
             request->m_body.empty() ? nullptr : &body_provider, request)) <
        0) {
      delete request;
      return socket_throw(
          topgg::exception::nghttp2("Unable to submit HTTP request", status));
    }

    m_ongoing_requests.push_back(request);

    m_frontend->m_requests_mutex.lock();
  }

  m_frontend->m_requests_mutex.unlock();

  TOPGG_LOG("Processing data from the remote peer");

  char buf[8 * 1024];
  int read_length{};

  while ((read_length = SSL_read(m_ssl, buf, sizeof(buf))) > 0) {
    if ((status = static_cast<int>(nghttp2_session_mem_recv2(
             m_nghttp2, reinterpret_cast<uint8_t*>(buf), read_length))) < 0) {
      return socket_throw(topgg::exception::nghttp2(
          "Unable to process data from the remote peer", status));
    }
  }

  const auto error{SSL_get_error(m_ssl, read_length)};

  if (error != SSL_ERROR_ZERO_RETURN && error != SSL_ERROR_WANT_READ &&
      error != SSL_ERROR_WANT_WRITE) {
    return socket_throw(
        topgg::exception::ssl("Unable to process data from the remote peer"));
  }

  TOPGG_LOG("Sending pending frames to the remote peer");

  if ((status = nghttp2_session_send(m_nghttp2)) < 0) {
    return socket_throw(topgg::exception::nghttp2(
        "Unable to send pending frames to the remote peer", status));
  }
}

nghttp2_ssize topgg::http_backend::on_send(
    [[maybe_unused]] nghttp2_session* http2, const uint8_t* data, size_t length,
    [[maybe_unused]] int flags, void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 SEND] %d bytes", length);

  auto self{reinterpret_cast<topgg::http_backend*>(ptr)};

  self->m_pending_writes.push_back(
      {std::vector<uint8_t>{data, data + length}, 0});

  while (!self->m_pending_writes.empty()) {
    auto& pending{self->m_pending_writes.front()};
    const auto write_length{
        SSL_write(self->m_ssl, pending.data.data() + pending.offset,
                  static_cast<int>(pending.data.size() - pending.offset))};

    if (write_length > 0) {
      pending.offset += write_length;

      if (pending.offset == pending.data.size()) {
        self->m_pending_writes.pop_front();
      }

      continue;
    }

    const auto error{SSL_get_error(self->m_ssl, write_length)};

    if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ) {
      TOPGG_LOGF("Temporarily pausing SSL_write from error %d", error);

      return NGHTTP2_ERR_WOULDBLOCK;
    }

    self->socket_throw(topgg::exception::ssl("Unable to write SSL data"));

    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  TOPGG_LOG("Flushing SSL_write");

  self->flush();

  return static_cast<nghttp2_ssize>(length);
}

int topgg::http_backend::on_header([[maybe_unused]] nghttp2_session* http2,
                                   const nghttp2_frame* frame,
                                   const uint8_t* name, size_t name_length,
                                   const uint8_t* value, size_t value_length,
                                   [[maybe_unused]] uint8_t flags, void* ptr) {
  if (frame->hd.type == NGHTTP2_HEADERS &&
      frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
    auto request{reinterpret_cast<topgg::http_request*>(
        nghttp2_session_get_stream_user_data(
            reinterpret_cast<topgg::http_backend*>(ptr)->m_nghttp2,
            frame->hd.stream_id))};

    TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d HEADER] %.*s: %.*s",
               frame->hd.stream_id, name_length, name, value_length, value);

    if (name_length == 7 && memcmp(name, ":status", 7) == 0 &&
        value_length >= 3) {
      request->m_status = (static_cast<uint16_t>(value[0] - '0') * 100) +
                          (static_cast<uint16_t>(value[1] - '0') * 10) +
                          static_cast<uint16_t>(value[2] - '0');
    } else if (name_length == 11 && memcmp(name, "retry-after", 11) == 0) {
      request->m_retry_after = std::stoi(
          std::string{reinterpret_cast<const char*>(value), value_length});
    }
  }

  return 0;
}

int topgg::http_backend::on_data_chunk([[maybe_unused]] nghttp2_session* http2,
                                       [[maybe_unused]] uint8_t flags,
                                       int32_t stream_id, const uint8_t* data,
                                       size_t length, void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d BODY CHUNK] %d bytes", stream_id,
             length);

  auto request{reinterpret_cast<topgg::http_request*>(
      nghttp2_session_get_stream_user_data(
          reinterpret_cast<topgg::http_backend*>(ptr)->m_nghttp2, stream_id))};

  request->m_response.append(
      reinterpret_cast<char*>(const_cast<uint8_t*>(data)), length);

  return 0;
}

void topgg::http_backend::dispatch(topgg::http_request* request,
                                   uv_work_cb work_callback) {
  auto work{new uv_work_t};

  work->data = request;

  uv_queue_work(m_loop, work, work_callback,
                [](uv_work_t* work, [[maybe_unused]] int status) {
                  delete reinterpret_cast<topgg::http_request*>(work->data);
                  delete work;
                });
}

int topgg::http_backend::on_stream_close(
    [[maybe_unused]] nghttp2_session* http2, int32_t stream_id, uint32_t error,
    void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d CLOSE] error: %d", stream_id, error);

  auto self{reinterpret_cast<topgg::http_backend*>(ptr)};
  auto request{reinterpret_cast<topgg::http_request*>(
      nghttp2_session_get_stream_user_data(self->m_nghttp2, stream_id))};

  self->m_ongoing_requests.erase(
      std::remove(self->m_ongoing_requests.begin(),
                  self->m_ongoing_requests.end(), request),
      self->m_ongoing_requests.end());

  if (error == 0) {
    self->dispatch(request, [](uv_work_t* work) {
      reinterpret_cast<topgg::http_request*>(work->data)->dispatch_response();
    });
  } else {
    self->dispatch(request, [](uv_work_t* work) {
      reinterpret_cast<topgg::http_request*>(work->data)
          ->dispatch_exception(
              "nghttp2 stream closed with non-zero error code");
    });
  }

  if (self->m_open_requests > 0) {
    self->m_open_requests--;
  }

  self->m_frontend->m_requests_mutex.lock();

  if (self->m_open_requests == 0 &&
      self->m_shutdown.load(std::memory_order::memory_order_acquire)) {
    const auto empty_incoming_requests{self->m_frontend->m_requests.empty()};

    self->m_frontend->m_requests_mutex.unlock();

    if (empty_incoming_requests) {
      self->shutdown();
    } else {
#ifndef TOPGG_PROJECT_TOKENS_ONLY
      self->flush_oauth2_requests();
#endif
      self->flush_requests();
    }
  } else {
    self->m_frontend->m_requests_mutex.unlock();
  }

  return 0;
}

#ifndef TOPGG_PROJECT_TOKENS_ONLY
void topgg::http_backend::flush_oauth2_requests() {
  TOPGG_LOG("Flushing oauth2 requests");

  m_oauth2_mutex.lock();

  while (!m_oauth2_refresh_queue.empty()) {
    const auto client{m_oauth2_refresh_queue.back()};

    m_oauth2_refresh_queue.pop_back();
    m_oauth2_clients.push_back(client);
    m_oauth2_mutex.unlock();

    uv_timer_init(m_loop, &client->m_oauth2_refresh_timer);
    client->m_oauth2_refresh_timer.data = new std::shared_ptr{client};

    const auto now{time(nullptr)};

    uv_timer_start(
        &client->m_oauth2_refresh_timer,
        [](uv_timer_t* timer) {
          TOPGG_LOG("Refreshing access token");

          auto client{reinterpret_cast<std::shared_ptr<topgg::oauth2_client>*>(
              timer->data)};

          (*client)->refresh_token();
        },
        now > (client->m_session.token_expires_at - 5000)
            ? 0
            : ((client->m_session.token_expires_at - now) * 1000) - 5000,
        (TOPGG_TOKEN_EXPIRY_INTERVAL * 1000) - 5000);

    m_oauth2_mutex.lock();
  }

  while (!m_oauth2_revoke_queue.empty()) {
    const auto client{m_oauth2_revoke_queue.back()};

    m_oauth2_revoke_queue.pop_back();
    m_oauth2_clients.erase(
        std::remove(m_oauth2_clients.begin(), m_oauth2_clients.end(), client),
        m_oauth2_clients.end());
    m_oauth2_mutex.unlock();

    client->stop_refresh_token();

    m_oauth2_mutex.lock();
  }

  m_oauth2_mutex.unlock();
}
#endif

void topgg::http_backend::on_async_flush_requests(uv_async_t* handle) {
  TOPGG_LOG("[EVENT: ASYNC FLUSH REQUESTS]");

  auto self{reinterpret_cast<topgg::http_backend*>(handle->data)};

  self->flush_requests();
}

#ifndef TOPGG_PROJECT_TOKENS_ONLY
void topgg::http_backend::on_async_flush_oauth2_requests(uv_async_t* handle) {
  TOPGG_LOG("[EVENT: ASYNC FLUSH OAUTH2 REQUESTS]");

  auto self{reinterpret_cast<topgg::http_backend*>(handle->data)};

  self->flush_oauth2_requests();
}
#endif

void topgg::http_backend::on_async_close(uv_async_t* handle) {
  TOPGG_LOG("[EVENT: ASYNC CLOSE]");

  auto self{reinterpret_cast<topgg::http_backend*>(handle->data)};

  if (self->m_open_requests == 0) {
    self->shutdown();
  }
}

void topgg::http_backend::on_close(uv_handle_t* handle) {
  TOPGG_LOG("Freeing up backend (1/2)");

  auto self{reinterpret_cast<topgg::http_backend*>(handle->data)};

  if (self->m_ssl != nullptr) {
    SSL_shutdown(self->m_ssl);
    self->m_ssl = nullptr;
  }

  if (self->m_nghttp2 != nullptr) {
    nghttp2_session_del(self->m_nghttp2);
    self->m_nghttp2 = nullptr;
  }

  if (self->m_ssl_write_bio != nullptr) {
    BIO_free(self->m_ssl_write_bio);
    self->m_ssl_write_bio = nullptr;
  }

  if (self->m_ssl_read_bio != nullptr) {
    BIO_free(self->m_ssl_read_bio);
    self->m_ssl_read_bio = nullptr;
  }

  self->m_shook_hand = false;
  self->m_socket.data = nullptr;
}

void topgg::http_backend::loop() {
  TOPGG_LOG("Running libuv event loop");

  uv_run(m_loop, UV_RUN_DEFAULT);
}

void topgg::http_backend::shutdown(const bool blocking) {
  TOPGG_LOGF("Shutting down backend (blocking: %d)", blocking);

  m_frontend->m_requests_mutex.lock();

  while (!m_frontend->m_requests.empty()) {
    const auto request{m_frontend->m_requests.back()};

    m_frontend->m_requests.pop_back();
    m_frontend->m_requests_mutex.unlock();

    if (m_error.has_value()) {
      request->dispatch_exception(m_error.value());
    }

    delete request;

    m_frontend->m_requests_mutex.lock();
  }

  m_frontend->m_requests_mutex.unlock();

  while (!m_ongoing_requests.empty()) {
    const auto request{m_ongoing_requests.back()};

    m_ongoing_requests.pop_back();

    if (m_error.has_value()) {
      request->dispatch_exception(m_error.value());
    }

    delete request;
  }

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  m_oauth2_mutex.lock();

  while (!m_oauth2_clients.empty()) {
    const auto client{m_oauth2_clients.back()};

    m_oauth2_clients.pop_back();
    m_oauth2_mutex.unlock();

    client->stop_refresh_token();

    m_oauth2_mutex.lock();
  }

  m_oauth2_mutex.unlock();
#endif

  if (m_async_close.data != nullptr) {
    uv_close(reinterpret_cast<uv_handle_t*>(&m_async_close), nullptr);
    m_async_close.data = nullptr;
  }

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  {
    std::lock_guard guard_{m_oauth2_mutex};

    if (m_async_flush_oauth2_requests.data != nullptr) {
      uv_close(reinterpret_cast<uv_handle_t*>(&m_async_flush_oauth2_requests),
               nullptr);
      m_async_flush_oauth2_requests.data = nullptr;
    }
  };
#endif

  {
    std::lock_guard guard_{m_flush_requests_mutex};

    if (m_async_flush_requests.data != nullptr) {
      uv_close(reinterpret_cast<uv_handle_t*>(&m_async_flush_requests),
               nullptr);
      m_async_flush_requests.data = nullptr;
    }
  };

  if (m_socket.data != nullptr &&
      !uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_socket))) {
    if (m_nghttp2 != nullptr) {
      nghttp2_session_terminate_session(m_nghttp2, NGHTTP2_NO_ERROR);
      nghttp2_session_send(m_nghttp2);
    }

    uv_close(reinterpret_cast<uv_handle_t*>(&m_socket), http_backend::on_close);
  }

  if (blocking) {
    loop();
  }
}

topgg::http_backend::~http_backend() {
  TOPGG_LOG("Freeing up backend (2/2)");

  shutdown(true);

  if (m_addrs != nullptr) {
    freeaddrinfo(m_addrs);
    m_addrs = nullptr;
  }

  if (m_ssl_context != nullptr) {
    SSL_CTX_free(m_ssl_context);
    m_ssl_context = nullptr;
  }

  if (m_loop != nullptr) {
    uv_loop_close(m_loop);
    m_loop = nullptr;
  }
}

topgg::http_request::http_request(const std::string& token,
                                  const std::string_view& method,
                                  const std::string& path,
                                  const topgg::http_request::callback& callback,
                                  const std::string& body,
                                  const std::string& content_type)
    : m_path("/api/v1" + path),
      m_content_type(content_type),
      m_content_length(std::to_string(body.size())),
      m_body(body),
      m_callback(callback) {
  add_header(":method", method);
  add_header(":scheme", "https");
  add_header(":authority", "top.gg");
  add_header(":path", m_path);
  add_header("content-length", m_content_length);
  add_header("content-type", m_content_type);
  add_header("user-agent", "topgg-cpp-sdk");

  if (!token.empty()) {
    m_authorization = "Bearer " + token;

    add_header("authorization", m_authorization);
  }
}

void topgg::http_request::add_header(const std::string_view& name,
                                     const std::string_view& value) {
  nghttp2_nv header{};

  header.flags = NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE;
  header.name =
      const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(name.data()));
  header.namelen = name.length();
  header.value =
      const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(value.data()));
  header.valuelen = value.length();

  m_headers.push_back(header);
}

ssize_t topgg::http_request::on_body_read(
    [[maybe_unused]] nghttp2_session* session,
    [[maybe_unused]] int32_t stream_id, uint8_t* data, size_t length,
    uint32_t* flags, nghttp2_data_source* source, [[maybe_unused]] void* ptr) {
  auto self{reinterpret_cast<topgg::http_request*>(source->ptr)};

  const auto remaining{self->m_body.length() - self->m_body_position};
  auto to_send{std::min(remaining, length)};

  TOPGG_LOGF("[EVENT: NGHTTP2 REQUEST BODY READ] sending %d bytes", to_send);

  if (to_send > 0) {
    memcpy(data, self->m_body.data() + self->m_body_position, to_send);
    self->m_body_position += to_send;
  }

  if (remaining == 0) {
    *flags |= NGHTTP2_DATA_FLAG_EOF;
  }

  return to_send;
}

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
topgg::http_request::~http_request() { TOPGG_LOG("Freeing up request"); }
#endif

topgg::http_frontend::http_frontend() {
  topgg::waker frontend_waker{};
  std::optional<topgg::exception> backend_init_error{std::nullopt};

  m_backend_thread = std::thread([this, &frontend_waker,
                                  &backend_init_error]() {
    TOPGG_LOG("Started backend thread");

    topgg::http_backend backend{this};

    m_backend = &backend;

    try {
      backend.init();
    } catch (const topgg::exception& error) {
      backend_init_error = error;
    }

    frontend_waker.notify();

    while (1) {
      backend.m_waker.wait();
      backend.m_frontend->m_requests_mutex.lock();

      if (backend.m_open_requests == 0 &&
          backend.m_frontend->m_requests.empty() &&
          backend.m_shutdown.load(std::memory_order::memory_order_acquire)) {
        backend.m_frontend->m_requests_mutex.unlock();
        break;
      }

      backend.m_frontend->m_requests_mutex.unlock();

      backend.connect();
      backend.loop();
    }
  });

  frontend_waker.wait();

  TOPGG_LOGF("Backend set up with error: %d", backend_init_error.has_value());

  if (backend_init_error.has_value()) {
    if (m_backend_thread.joinable()) {
      m_backend_thread.join();
    }

    throw backend_init_error.value();
  }
}

void topgg::http_frontend::fetch(topgg::http_request* request,
                                 const bool defer) {
  std::lock_guard guard_{m_requests_mutex};

  m_requests.push_back(request);

  if (!defer) {
    std::lock_guard guard_{m_backend->m_flush_requests_mutex};

    if (m_backend->m_async_flush_requests.data != nullptr) {
      uv_async_send(&m_backend->m_async_flush_requests);
    } else {
      m_backend->m_waker.notify();
    }
  }
}

topgg::http_frontend::~http_frontend() {
  TOPGG_LOG("Freeing up frontend");

  m_backend->m_shutdown.store(true, std::memory_order::memory_order_release);

  if (m_backend->m_async_close.data != nullptr) {
    uv_async_send(&m_backend->m_async_close);
  }

  m_backend->m_waker.notify();

  if (m_backend_thread.joinable()) {
    m_backend_thread.join();
  }
}
