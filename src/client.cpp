#include <topgg/topgg.h>

#include <algorithm>

#ifndef TOPGG_PROJECT_TOKENS_ONLY
#define TOPGG_NEW_TOKEN_EXPIRY_TIMESTAMP() \
  (time(nullptr) + TOPGG_TOKEN_EXPIRY_INTERVAL)
#endif

void topgg::base_client::fetch_empty(const std::string_view& method,
                                     const std::string& path,
                                     const topgg::empty_callback& callback,
                                     const bool defer,
                                     const std::string& body) {
  get_http()->fetch(
      new topgg::http_request{
          get_token(), method, path,
          [callback](const topgg::http_response_pair& response_pair) {
            if (std::holds_alternative<topgg::exception>(response_pair)) {
              callback(std::get<topgg::exception>(response_pair));
            } else {
              try {
                const auto& response{
                    std::get<topgg::http_response>(response_pair)};

                if (response.status >= 400) {
                  callback(topgg::http_exception{response});
                } else {
                  callback(std::monostate{});
                }
              } catch (const nlohmann::json::exception& error) {
                callback(error);
              }
            }
          },
          body},
      defer);
}

void topgg::base_client::get_projects(
    const topgg::paginated_callback<topgg::partial_project>& callback,
    const bool defer) {
  fetch_paginated("projects", "GET", "/projects", callback, defer);
}

void topgg::base_client::get_projects(
    const topgg::paginated_result<topgg::partial_project>& cursor,
    const topgg::paginated_callback<topgg::partial_project>& callback,
    const bool defer) {
  fetch_paginated(cursor, "projects", "GET", "/projects", callback, defer);
}

void topgg::base_client::get_project(
    const std::string& id, const topgg::callback<topgg::project>& callback,
    const bool defer) {
  fetch_simple("GET", "/projects/" + id, callback, defer);
}

void topgg::base_client::edit_project(const std::string& id,
                                      const topgg::locale_map& headline,
                                      const topgg::locale_map& page_content,
                                      const topgg::empty_callback& callback,
                                      const bool defer) {
  if (headline.m_json.empty() && page_content.m_json.empty()) {
    throw topgg::exception{"Either headline or page_content must be specified"};
  }

  headline.enforce_constraints(3, 140);
  page_content.enforce_constraints(300, 50000);

  nlohmann::json body{};

  body["headline"] = headline.m_json;
  body["page_content"] = page_content.m_json;

  fetch_empty("PATCH", "/projects/" + id, callback, defer, body.dump());
}

void topgg::base_client::post_announcement(
    const std::string& id, const std::string& title, const std::string& content,
    const topgg::announcement_category category,
    const topgg::empty_callback& callback, const bool defer) {
  if (title.length() < 3 || title.length() > 100) {
    throw topgg::exception{"Title length is outside of the accepted threshold"};
  } else if (content.length() < 100 || content.length() > 2000) {
    throw topgg::exception{
        "Content length is outside of the accepted threshold"};
  }

  nlohmann::json body{};

  body["title"] = title;
  body["content"] = content;

  std::string category_string{};

  switch (category) {
    case topgg::announcement_category::ac_announcement:
      category_string = "announcement";
      break;
    case topgg::announcement_category::ac_event:
      category_string = "event";
      break;
    case topgg::announcement_category::ac_new_feature:
      category_string = "new_feature";
      break;
  }

  body["category"] = category_string;

  fetch_empty("POST", "/projects/" + id + "/announcements", callback, defer,
              body.dump());
}

void topgg::base_client::post_metrics(const std::string& id,
                                      const topgg::metrics& metrics,
                                      const topgg::empty_callback& callback,
                                      const bool defer) {
  fetch_empty("PATCH", "/projects/" + id + "/metrics", callback, defer,
              metrics.to_json().dump());
}

void topgg::base_client::post_commands(const std::string& id,
                                       const std::string& commands,
                                       const topgg::empty_callback& callback,
                                       const bool defer) {
  fetch_empty("PUT", "/projects/" + id + "/commands", callback, defer,
              commands);
}

void topgg::base_client::get_votes(
    const std::string& id, const time_t since,
    const topgg::paginated_callback<topgg::vote>& callback, const bool defer) {
  const auto now{time(nullptr)};

  if (now < since || (now - since) > 31536000) {
    throw topgg::exception{"Invalid since timestamp"};
  }

  fetch_paginated("data", "GET",
                  "/projects/" + id + "/votes?startDate=" +
                      topgg::_url_encode(topgg::_to_time_string(since)),
                  callback, defer);
}

void topgg::base_client::get_votes(
    const std::string& id, const topgg::paginated_result<topgg::vote>& cursor,
    const topgg::paginated_callback<topgg::vote>& callback, const bool defer) {
  fetch_paginated(cursor, "data", "GET", "/projects/" + id + "/votes", callback,
                  defer);
}

void topgg::base_client::get_vote(
    const std::string& id, const std::string& user_id,
    const topgg::user_source& source,
    const topgg::callback<topgg::partial_vote>& callback, const bool defer) {
  std::string source_string{};

  switch (source) {
    case topgg::user_source::us_topgg:
      source_string = "topgg";
      break;
    case topgg::user_source::us_discord:
      source_string = "discord";
      break;
  }

  fetch_simple(
      "GET",
      "/projects/" + id + "/votes/" + user_id + "?source=" + source_string,
      callback, defer);
}

void topgg::base_client::get_integrations(
    const std::string& id,
    const topgg::callback<std::vector<topgg::integration>>& callback,
    const bool defer) {
  fetch_vector("GET", "/projects/" + id + "/integrations", callback, defer);
}

void topgg::base_client::connect_integration(
    const std::string& project_id, const std::string& integration_id,
    const topgg::empty_callback& callback, const bool defer) {
  fetch_empty("PUT",
              "/projects/" + project_id + "/integrations/" + integration_id,
              callback, defer);
}

void topgg::base_client::disconnect_integration(
    const std::string& project_id, const std::string& integration_id,
    const topgg::empty_callback& callback, const bool defer) {
  fetch_empty("DELETE",
              "/projects/" + project_id + "/integrations/" + integration_id,
              callback, defer);
}

void topgg::base_client::get_webhooks(
    const std::string& id,
    const topgg::callback<std::vector<topgg::webhook>>& callback,
    const bool defer) {
  fetch_vector("GET", "/projects/" + id + "/webhooks", callback, defer);
}

void topgg::base_client::create_webhook(const std::string& id,
                                        const topgg::base_webhook& webhook,
                                        const topgg::empty_callback& callback,
                                        const bool defer) {
  nlohmann::json body{};

  body["label"] = webhook.label;
  body["url"] = webhook.url;

  fetch_empty("POST", "/projects/" + id + "/webhooks", callback, defer,
              body.dump());
}

void topgg::base_client::delete_webhook(const std::string& project_id,
                                        const std::string& webhook_id,
                                        const topgg::empty_callback& callback,
                                        const bool defer) {
  fetch_empty("DELETE", "/projects/" + project_id + "/webhooks/" + webhook_id,
              callback, defer);
}

void topgg::base_client::rotate_webhook_secret(
    const std::string& project_id, const std::string& webhook_id,
    const topgg::callback<std::string>& callback, const bool defer) {
  get_http()->fetch(
      new http_request{
          get_token(), "POST",
          "/projects/" + project_id + "/webhooks/" + webhook_id + "/rotate",
          [callback](const topgg::http_response_pair& response_pair) {
            if (std::holds_alternative<exception>(response_pair)) {
              callback(std::get<exception>(response_pair));
            } else {
              try {
                const auto& response{
                    std::get<topgg::http_response>(response_pair)};

                if (response.status >= 400) {
                  callback(http_exception{response});
                } else {
                  const auto json{nlohmann::json::parse(response.body)};

                  callback(json["secret"].template get<std::string>());
                }
              } catch (const nlohmann::json::exception& error) {
                callback(error);
              }
            }
          }},
      defer);
}

void topgg::base_client::test_webhook(const std::string& project_id,
                                      const std::string& webhook_id,
                                      const topgg::empty_callback& callback,
                                      const bool defer) {
  fetch_empty("POST",
              "/projects/" + project_id + "/webhooks/" + webhook_id + "/test",
              callback, defer);
}

#ifndef TOPGG_PROJECT_TOKENS_ONLY
topgg::oauth2_session topgg::oauth2_client::get_session() {
  std::lock_guard guard_{m_token_mutex};

  return m_session;
}

bool topgg::oauth2_client::has_expired() {
  std::lock_guard guard_{m_token_mutex};

  return m_session.has_expired();
}

std::string topgg::oauth2_client::get_token() {
  std::lock_guard guard_{m_token_mutex};

  return m_session.token;
}

void topgg::oauth2_client::refresh_token() {
  std::string body{"grant_type=refresh_token&client_id="};

  {
    std::lock_guard guard_{m_token_mutex};

    body += m_oauth2->m_client_id +
            "&client_secret=" + m_oauth2->m_client_secret +
            "&refresh_token=" + m_session.refresh_token;
  };

  get_http()->fetch(new topgg::http_request{
      "", "POST", "/oauth2/token",
      [this](const topgg::http_response_pair& response_pair) {
        if (std::holds_alternative<topgg::http_response>(response_pair)) {
          const auto& response{std::get<topgg::http_response>(response_pair)};

          if (response.status < 400) {
            try {
              const auto json{nlohmann::json::parse(response.body)};

              std::lock_guard guard_{m_token_mutex};

              auto token{json["access_token"].template get<std::string>()};
              auto refresh_token{
                  json["refresh_token"].template get<std::string>()};

              m_session = topgg::oauth2_session{
                  token, refresh_token, TOPGG_NEW_TOKEN_EXPIRY_TIMESTAMP()};
            } catch (const nlohmann::json::exception&) {
            }
          }
        }
      },
      body, "application/x-www-form-urlencoded"});
}

void topgg::oauth2_client::stop_refresh_token() {
  if (m_oauth2_refresh_timer.data != nullptr) {
    uv_timer_stop(&m_oauth2_refresh_timer);
    uv_close(
        reinterpret_cast<uv_handle_t*>(&m_oauth2_refresh_timer),
        [](uv_handle_t* handle) {
          auto client{reinterpret_cast<std::shared_ptr<topgg::oauth2_client>*>(
              handle->data)};

          (*client)->m_oauth2_refresh_timer.data = nullptr;

          delete client;
        });
  }
}

void topgg::oauth2_client::get_authorized_user(
    const topgg::callback<topgg::user>& callback, const bool defer) {
  fetch_simple("GET", "/users/@me", callback, defer);
}

void topgg::oauth2_client::get_authorized_user_projects(
    const topgg::callback<std::vector<topgg::user_project>>& callback,
    const bool defer) {
  fetch_vector("GET", "/users/@me/projects", callback, defer);
}

void topgg::oauth2_client::submit_project(
    const topgg::project_submission& submission,
    const topgg::empty_callback& callback, const bool defer) {
  nlohmann::json body{};

  std::string platform_string{};
  std::string project_type_string{};

  switch (submission.platform) {
    case topgg::platform::p_discord:
      platform_string = "discord";
      break;
    case topgg::platform::p_roblox:
      platform_string = "roblox";
      break;
  }

  switch (submission.type) {
    case topgg::project_type::pt_bot:
      project_type_string = "bot";
      break;
    case topgg::project_type::pt_server:
      project_type_string = "server";
      break;
    case topgg::project_type::pt_game:
      project_type_string = "game";
      break;
  }

  body["platform"] = platform_string;
  body["type"] = project_type_string;
  body["platform_id"] = submission.platform_id;

  if (submission.headline.length() < 3 || submission.headline.length() > 140) {
    throw topgg::exception{
        "Headline length is outside of the accepted threshold"};
  } else if (submission.page_content.length() < 300 ||
             submission.page_content.length() > 50000) {
    throw topgg::exception{
        "Page content length is outside of the accepted threshold"};
  }

  body["headline"] = submission.headline;
  body["page_content"] = submission.page_content;

  fetch_empty("POST", "/users/@me/projects", callback, defer, body.dump());
}

void topgg::oauth2_client::revoke_token(const topgg::empty_callback& callback,
                                        const bool defer) {
  std::string body{"client_id="};

  {
    std::lock_guard guard_{m_token_mutex};

    body += m_oauth2->m_client_id +
            "&client_secret=" + m_oauth2->m_client_secret +
            "&refresh_token=" + m_session.refresh_token;
  };

  auto http{get_http()};

  http->fetch(
      new topgg::http_request{
          "", "POST", "/oauth2/revoke",
          [callback](const topgg::http_response_pair& response_pair) {
            if (std::holds_alternative<topgg::exception>(response_pair)) {
              callback(std::get<topgg::exception>(response_pair));
            } else {
              try {
                const auto& response{
                    std::get<topgg::http_response>(response_pair)};

                if (response.status >= 400) {
                  callback(topgg::http_exception{response});
                } else {
                  callback(std::monostate{});
                }
              } catch (const nlohmann::json::exception& error) {
                callback(error);
              }
            }
          },
          body, "application/x-www-form-urlencoded"},
      defer);

  std::lock_guard guard_{http->m_backend->m_oauth2_mutex};

  http->m_backend->m_oauth2_revoke_queue.push_back(shared_from_this());

  if (http->m_backend->m_async_flush_oauth2_requests.data != nullptr) {
    uv_async_send(&http->m_backend->m_async_flush_oauth2_requests);
  } else {
    http->m_backend->m_waker.notify();
  }
}

void topgg::oauth2_url::exchange(
    const std::string& code,
    const topgg::callback<topgg::oauth2_session>& callback) {
  std::string body{"grant_type=authorization_code&client_id="};

  body += m_oauth2->m_client_id +
          "&client_secret=" + m_oauth2->m_client_secret + "&code=" + topgg::_url_encode(code) +
          "&redirect_uri=" + topgg::_url_encode(m_oauth2->m_redirect_uri) +
          "&code_verifier=" + topgg::_url_encode(code_verifier);

  m_oauth2->m_http.fetch(new topgg::http_request{
      "", "POST", "/oauth2/token",
      [this, callback](const topgg::http_response_pair& response_pair) {
        if (std::holds_alternative<topgg::exception>(response_pair)) {
          callback(std::get<topgg::exception>(response_pair));
        } else {
          try {
            const auto& response{std::get<topgg::http_response>(response_pair)};

            if (response.status >= 400) {
              callback(topgg::http_exception{response});
            } else {
              const auto json{nlohmann::json::parse(response.body)};

              callback(topgg::oauth2_session{
                  json["access_token"].template get<std::string>(),
                  json["refresh_token"].template get<std::string>(),
                  TOPGG_NEW_TOKEN_EXPIRY_TIMESTAMP()});
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        }
      },
      body, "application/x-www-form-urlencoded"});
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif

topgg::oauth2::oauth2(const std::string& client_id,
                      const std::string& client_secret,
                      const std::string& redirect_uri,
                      const std::initializer_list<std::string_view>& scopes)
    : m_client_id(client_id),
      m_client_secret(client_secret),
      m_redirect_uri(redirect_uri) {
  for (const auto& scope : scopes) {
    m_scopes += '+';
    m_scopes += scope;
  }

  if ((m_bio = BIO_new(BIO_s_mem())) == nullptr ||
      (m_base64_bio = BIO_new(BIO_f_base64())) == nullptr) {
    throw topgg::exception::ssl("Unable to create BIO");
  } else if ((m_md = EVP_MD_fetch(nullptr, "SHA256", nullptr)) == nullptr ||
             (m_md_context = EVP_MD_CTX_new()) == nullptr ||
             EVP_DigestInit_ex(m_md_context, m_md, nullptr) != 1) {
    throw topgg::exception::ssl("Unable to create SHA256 digest context");
  }

  BIO_set_flags(m_base64_bio, BIO_FLAGS_BASE64_NO_NL);
  m_base64_bio = BIO_push(m_base64_bio, m_bio);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

std::shared_ptr<topgg::oauth2_client> topgg::oauth2::new_client(
    const topgg::oauth2_session& session) {
  std::lock_guard guard_{m_http.m_backend->m_oauth2_mutex};

  for (const auto& client : m_http.m_backend->m_oauth2_clients) {
    if (client->get_token() == session.token) {
      return client;
    }
  }

  auto client{std::shared_ptr<topgg::oauth2_client>{
      new topgg::oauth2_client{this, session}}};

  m_http.m_backend->m_oauth2_refresh_queue.push_back(std::shared_ptr{client});

  if (m_http.m_backend->m_async_flush_oauth2_requests.data != nullptr) {
    uv_async_send(&m_http.m_backend->m_async_flush_oauth2_requests);
  } else {
    m_http.m_backend->m_waker.notify();
  }

  return client;
}

std::shared_ptr<topgg::oauth2_url> topgg::oauth2::new_url(const std::string& state, const std::string& code_verifier) {
  std::lock_guard guard_{m_url_mutex};

  if (EVP_DigestUpdate(m_md_context, code_verifier.data(),
                       code_verifier.length()) != 1) {
    throw topgg::exception{"Unable to update sha256 hash of code verifier"};
  }

  std::vector<uint8_t> code_verifier_hash(
      static_cast<size_t>(EVP_MD_get_size(m_md)));
  BUF_MEM* code_challenge_hash_base64{};
  unsigned int digest_size{};

  if (EVP_DigestFinal_ex(m_md_context, code_verifier_hash.data(),
                         &digest_size) != 1 ||
      BIO_write(m_base64_bio, code_verifier_hash.data(),
                static_cast<int>(code_verifier_hash.size())) < 1 ||
      BIO_flush(m_base64_bio) != 1) {
    throw topgg::exception{"Unable to compute sha256 digest of code verifier"};
  } else if (EVP_MD_CTX_reset(m_md_context) != 1 ||
             EVP_DigestInit_ex(m_md_context, m_md, nullptr) != 1) {
    throw topgg::exception{"Unable to reuse sha256 digest context"};
  }

  BIO_get_mem_ptr(m_bio, &code_challenge_hash_base64);

  auto code_challenge{std::string{code_challenge_hash_base64->data,
                                  code_challenge_hash_base64->length}};

  std::replace(code_challenge.begin(), code_challenge.end(), '+', '-');
  std::replace(code_challenge.begin(), code_challenge.end(), '/', '_');

  code_challenge.erase(
      std::remove(code_challenge.begin(), code_challenge.end(), '='),
      code_challenge.end());

  std::string_view scopes{m_scopes};

  scopes = scopes.empty() ? scopes : scopes.substr(1);

  auto url{std::string{"https://top.gg/oauth2/"
                       "authorize?response_type=code&code_challenge_method="
                       "S256&client_id="} +
           m_client_id + "&redirect_uri=" + topgg::_url_encode(m_redirect_uri) +
           "&state=" + topgg::_url_encode(state) + "&code_challenge=" + topgg::_url_encode(code_challenge) + "&scope="};

  url += scopes;

  return std::shared_ptr<topgg::oauth2_url>{
      new topgg::oauth2_url{this, state, code_verifier, url}};
}

std::shared_ptr<topgg::oauth2_url> topgg::oauth2::new_url() {
  const auto state{topgg::_random_string()};
  auto code_verifier{topgg::_random_string()};

  return new_url(state, code_verifier);
}

topgg::oauth2::~oauth2() {
  if (m_md_context != nullptr) {
    EVP_MD_CTX_free(m_md_context);
    m_md_context = nullptr;
  }

  if (m_md != nullptr) {
    EVP_MD_free(m_md);
    m_md = nullptr;
  }

  if (m_base64_bio != nullptr) {
    BIO_free(m_base64_bio);
    m_base64_bio = nullptr;
  }

  if (m_bio != nullptr) {
    BIO_free(m_bio);
    m_bio = nullptr;
  }
}

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
topgg::oauth2_client::~oauth2_client() {
  TOPGG_LOG("Freeing up oauth2 client");
}
#endif
#endif

#ifdef TOPGG_WEBHOOKS
topgg::webhooks::payload topgg::webhooks::parse(const std::string& body) {
  const auto json{nlohmann::json::parse(body)};
  const auto type{json["type"].template get<std::string>()};
  const auto data{json["data"]};

  if (type == "vote.create") {
    return topgg::webhooks::vote_create{data};
  } else if (type == "webhook.test") {
    return topgg::webhooks::test{data};
  } else if (type == "integration.create") {
    return topgg::webhooks::integration_create{data};
  } else if (type == "integration.delete") {
    return topgg::webhooks::integration_delete{data};
  }

  throw topgg::exception{"Got an unrecognized event type"};
}

topgg::webhooks::verifier::verifier() {
  if ((m_mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr)) == nullptr ||
      (m_mac_context = EVP_MAC_CTX_new(m_mac)) == nullptr) {
    throw topgg::exception::ssl("Unable to create HMAC context");
  }

  char sha256[] = "SHA256";

  m_params[0] = OSSL_PARAM_construct_utf8_string("digest", sha256, 0);
  m_params[1] = OSSL_PARAM_construct_end();
}

static const char g_hex_digits[] = "0123456789abcdef";

bool topgg::webhooks::verifier::verify(const std::string& secret,
                                       const std::string& signature,
                                       const std::string& body) {
  try {
    const auto signature_comma_position{signature.find(',')};

    if (signature_comma_position == std::string::npos) {
      return false;
    }

    const auto signature_timestamp_pair{
        signature.substr(0, signature_comma_position)};
    const auto signature_timestamp_equal_position{
        signature_timestamp_pair.find('=')};

    if (signature_timestamp_equal_position == std::string::npos) {
      return false;
    }

    const auto signature_timestamp_text{
        signature_timestamp_pair.substr(signature_timestamp_equal_position + 1,
                                        signature_timestamp_pair.length())};
    size_t signature_timestamp_pos{};
    int64_t signature_timestamp{
        std::stoll(signature_timestamp_text, &signature_timestamp_pos)};

    if (signature_timestamp_pos != signature_timestamp_text.size() ||
        (abs(static_cast<int64_t>(time(nullptr)) - signature_timestamp) >
         300)) {
      return false;
    }

    const auto hmac_update_string{signature_timestamp_text + '.' + body};

    const auto signature_signature_pair{
        signature.substr(signature_comma_position + 1, signature.length())};
    const auto signature_signature_equal_position{
        signature_signature_pair.find('=')};

    if (signature_signature_equal_position == std::string::npos) {
      return false;
    }

    const auto signature_signature{
        signature_signature_pair.substr(signature_signature_equal_position + 1,
                                        signature_signature_pair.length())};

    size_t hmac_length{};
    std::vector<uint8_t> hmac(hmac_length);

    {
      std::lock_guard guard_{m_mutex};

      if (EVP_MAC_init(m_mac_context,
                       reinterpret_cast<const unsigned char*>(secret.c_str()),
                       secret.length(), m_params) != 1 ||
          EVP_MAC_update(m_mac_context,
                         reinterpret_cast<const unsigned char*>(
                             hmac_update_string.c_str()),
                         hmac_update_string.length()) != 1 ||
          EVP_MAC_final(m_mac_context, nullptr, &hmac_length, 0) != 1 ||
          (hmac_length * 2) != signature_signature.length() ||
          EVP_MAC_final(m_mac_context, hmac.data(), &hmac_length,
                        hmac_length) != 1) {
        return false;
      }
    }

    uint8_t result{};
    size_t index{};

    for (const auto byte : hmac) {
      result |= (signature_signature[index++] ^ g_hex_digits[byte >> 4]);
      result |= (signature_signature[index++] ^ g_hex_digits[byte & 0x0f]);
    }

    return result == 0;
  } catch (const std::exception&) {
    return false;
  }
}

topgg::webhooks::verifier::~verifier() {
  if (m_mac_context != nullptr) {
    EVP_MAC_CTX_free(m_mac_context);
    m_mac_context = nullptr;
  }

  if (m_mac != nullptr) {
    EVP_MAC_free(m_mac);
    m_mac = nullptr;
  }
}
#endif
