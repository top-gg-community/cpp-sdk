#pragma once

#include <topgg/topgg.h>

#include <functional>
#include <string>

#ifndef TOPGG_PROJECT_TOKENS_ONLY
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <uv.h>

#include <ctime>
#include <memory>
#include <mutex>
#endif

namespace topgg {
/**
 * @brief An API call callback function that receives a topgg::result<T>.
 *
 * @since 2.0.0
 */
template <class T>
using callback = std::function<void(const result<T>&)>;

/**
 * @brief An API call callback function that receives a topgg::empty_result.
 *
 * @since 2.0.0
 */
using empty_callback = std::function<void(const empty_result&)>;
template <class T>

/**
 * @brief An API call callback function that receives a
 * topgg::paginated_result<T>.
 *
 * @since 2.0.0
 */
using paginated_callback = std::function<void(const paginated_result<T>&)>;

/**
 * @brief A base API client.
 *
 * @since 2.0.0
 */
class base_client {
 protected:
  virtual http_frontend* get_http() = 0;

  virtual std::string get_token() = 0;

  template <class T>
  void fetch_simple(const std::string_view& method, const std::string& path,
                    const callback<T>& callback_, const bool defer,
                    const std::string& body = "") {
    get_http()->fetch(
        new http_request{
            get_token(), method, path,
            [callback_](const http_response_pair& response_pair) {
              if (std::holds_alternative<exception>(response_pair)) {
                callback_(std::get<exception>(response_pair));
              } else {
                try {
                  const auto& response{std::get<http_response>(response_pair)};

                  if (response.status >= 400) {
                    callback_(http_exception{response});
                  } else {
                    callback_(T{nlohmann::json::parse(response.body)});
                  }
                } catch (const nlohmann::json::exception& error) {
                  callback_(error);
                }
              }
            },
            body},
        defer);
  }

  template <class T>
  void fetch_paginated(const char* key, const std::string_view& method,
                       const std::string& path,
                       const paginated_callback<T>& callback,
                       const bool defer) {
    get_http()->fetch(
        new http_request{
            get_token(), method, path,
            [key, callback](const http_response_pair& response_pair) {
              if (std::holds_alternative<exception>(response_pair)) {
                callback(std::get<exception>(response_pair));
              } else {
                try {
                  const auto& response{std::get<http_response>(response_pair)};

                  if (response.status >= 400) {
                    callback(http_exception{response});
                  } else {
                    callback(paginated_result<T>::from_array(
                        key, nlohmann::json::parse(response.body)));
                  }
                } catch (const nlohmann::json::exception& error) {
                  callback(error);
                }
              }
            }},
        defer);
  }

  template <class T>
  void fetch_paginated(const paginated_result<T>& cursor_, const char* key,
                       const std::string_view& method, const std::string& path,
                       const paginated_callback<T>& callback,
                       const bool defer) {
    const auto cursor{cursor_.cursor()};

    if (cursor.has_value()) {
      fetch_paginated(key, method, path + "?cursor=" + cursor.value(), callback,
                      defer);
    } else {
      callback(paginated_result<T>::empty());
    }
  }

  void fetch_empty(const std::string_view& method, const std::string& path,
                   const empty_callback& callback, const bool defer,
                   const std::string& body = "");

  template <class T>
  void fetch_vector(const std::string_view& method, const std::string& path,
                    const callback<std::vector<T>>& callback, const bool defer,
                    const std::string& body = "") {
    get_http()->fetch(
        new http_request{
            get_token(), method, path,
            [callback](const http_response_pair& response_pair) {
              if (std::holds_alternative<exception>(response_pair)) {
                callback(std::get<exception>(response_pair));
              } else {
                try {
                  const auto& response{std::get<http_response>(response_pair)};

                  if (response.status >= 400) {
                    callback(http_exception{response});
                  } else {
                    const auto json{nlohmann::json::parse(response.body)};
                    std::vector<T> output{};

                    for (const auto& project : json) {
                      output.push_back(T{project});
                    }

                    callback(output);
                  }
                } catch (const nlohmann::json::exception& error) {
                  callback(error);
                }
              }
            },
            body},
        defer);
  }

 public:
  /**
   * @brief Fetches the projects the current credential covers.
   *
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_projects(const paginated_callback<partial_project>& callback,
                    const bool defer = false);

  /**
   * @brief Fetches the projects the current credential covers from a cursor.
   *
   * @param cursor The cursor frame of reference.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_projects(const paginated_result<partial_project>& cursor,
                    const paginated_callback<partial_project>& callback,
                    const bool defer = false);

  /**
   * @brief Fetches a project associated with the current token.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_project(const std::string& id, const callback<project>& callback,
                   const bool defer = false);

  /**
   * @brief Updates the headline and/or page content for a project. Both fields
   * are locale-keyed, so you can set content for multiple languages in a single
   * request.
   *
   * @param id The project's Top.gg ID.
   * @param headline A map of locales to headline strings. Each headline must be
   * between 3 and 140 characters.
   * @param page_content A map of locales to page content strings (Markdown
   * supported.) Each entry must be between 300 and 50,000 characters.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throw topgg::exception The headline or page_content length does not match
   * the required thresholds.
   * @since 2.0.0
   */
  void edit_project(const std::string& id, const locale_map& headline,
                    const locale_map& page_content,
                    const empty_callback& callback, const bool defer = false);

  /**
   * @brief Creates a new announcement for a project. Announcements appear on a
   * project’s page and can be used to notify users about updates, new features,
   * or other news.
   *
   * @param id The project's Top.gg ID.
   * @param title The announcement's title.
   * @param content The announcement's body.
   * @param category The category to publish the announcement under.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throw topgg::exception The title or content length does not match the
   * required thresholds.
   * @since 2.0.0
   */
  void post_announcement(const std::string& id, const std::string& title,
                         const std::string& content,
                         const announcement_category category,
                         const empty_callback& callback,
                         const bool defer = false);

  /**
   * @brief Submits a single metrics payload for a project. Use this to push
   * fresh numbers after an event such as joining or leaving a guild or a player
   * connecting.
   *
   * @param id The project's Top.gg ID.
   * @param metrics The metrics payload.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void post_metrics(const std::string& id, const metrics& metrics,
                    const empty_callback& callback, const bool defer = false);

  /**
   * @brief Submits up to 100 metrics entries in a single request.
   *
   * @param id The project's Top.gg ID.
   * @param metrics_ The metrics payloads.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throws topgg::exception The metrics entries length does not match the
   * required threshold.
   * @since 2.0.0
   */
  template <class T>
  void post_metrics(const std::string& id,
                    const timestamped_metrics<T>& metrics_,
                    const empty_callback& callback, const bool defer = false) {
    if (metrics_.m_json.size() < 1 || metrics_.m_json.size() > 100) {
      throw exception{"Batch length is outside of the accepted threshold"};
    }

    nlohmann::json body{};

    body["data"] = metrics_.m_json;

    fetch_empty("POST", "/projects/" + id + "/metrics/batch", callback, defer,
                body.dump());
  }

  /**
   * @brief Overwrites the list of slash command definitions for a bot project
   * on Top.gg. This is only applicable to bot-type projects on the discord
   * platform.
   *
   * @param id The project's Top.gg ID.
   * @param commands The array of slash commands in a JSON-stringified Discord
   * API's application command structure format.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void post_commands(const std::string& id, const std::string& commands,
                     const empty_callback& callback, const bool defer = false);

  /**
   * @brief Fetches a cursor-paginated list of votes for a project, ordered by
   * creation date (oldest first within each page.)
   *
   * @param id The project's Top.gg ID.
   * @param since The creation date frame of reference.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throws topgg::exception The creation date frame of reference is invalid.
   * @since 2.0.0
   */
  void get_votes(const std::string& id, const time_t since,
                 const paginated_callback<vote>& callback,
                 const bool defer = false);

  /**
   * @brief Fetches a cursor-paginated list of votes for a project from a
   * cursor.
   *
   * @param id The project's Top.gg ID.
   * @param cursor The cursor frame of reference.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_votes(const std::string& id, const paginated_result<vote>& cursor,
                 const paginated_callback<vote>& callback,
                 const bool defer = false);

  /**
   * @brief Fetches the most recent vote for a specific user.
   *
   * @param id The project's Top.gg ID.
   * @param user_id The ID of the user to look up. The expected format depends
   * on the source parameter.
   * @param source The ID type being provided.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_vote(const std::string& id, const std::string& user_id,
                const user_source& source,
                const callback<partial_vote>& callback,
                const bool defer = false);

  /**
   * @brief Fetches the integrations available for a project and whether each
   * one is connected.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_integrations(const std::string& id,
                        const callback<std::vector<integration>>& callback,
                        const bool defer = false);

  /**
   * @brief Connects an integration to the project. Top.gg runs the integration
   * handshake and starts delivering events to it.
   *
   * @param project_id The project's Top.gg ID.
   * @param integration_id The integration's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void connect_integration(const std::string& project_id,
                           const std::string& integration_id,
                           const empty_callback& callback,
                           const bool defer = false);

  /**
   * @brief Disconnects an integration the application connected. The
   * integration receives an integration.delete event.
   *
   * @param project_id The project's Top.gg ID.
   * @param integration_id The integration's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void disconnect_integration(const std::string& project_id,
                              const std::string& integration_id,
                              const empty_callback& callback,
                              const bool defer = false);

  /**
   * @brief Fetches the webhooks the application created on the project.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_webhooks(const std::string& id,
                    const callback<std::vector<webhook>>& callback,
                    const bool defer = false);

  /**
   * @brief Creates a vote webhook on the project.
   *
   * @param id The project's Top.gg ID.
   * @param webhook The vote webhook.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void create_webhook(const std::string& id, const base_webhook& webhook,
                      const empty_callback& callback, const bool defer = false);

  /**
   * @brief Deletes a webhook the application created.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void delete_webhook(const std::string& project_id,
                      const std::string& webhook_id,
                      const empty_callback& callback, const bool defer = false);

  /**
   * @brief Replaces the signing secret of a webhook the application created.
   * The previous secret stops working immediately.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void rotate_webhook_secret(const std::string& project_id,
                             const std::string& webhook_id,
                             const callback<std::string>& callback,
                             const bool defer = false);

  /**
   * @brief Sends a webhook.test event to a webhook the application created.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void test_webhook(const std::string& project_id,
                    const std::string& webhook_id,
                    const empty_callback& callback, const bool defer = false);
};

#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
/**
 * @brief A project or application API client.
 *
 * @since 2.0.0
 */
class client : public base_client {
  http_frontend m_http{};
  std::string m_token{};

  inline http_frontend* get_http() override { return &m_http; }

  inline std::string get_token() override { return m_token; }

 public:
  client() = delete;

  /**
   * @brief Creates a client instance.
   *
   * @param token The project or application token.
   * @since 2.0.0
   */
  inline client(const std::string& token) : m_token(token) {}

  /**
   * @brief Fetches the project associated with the current token.
   *
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  inline void get_project(const callback<project>& callback,
                          const bool defer = false) {
    base_client::get_project("@me", callback, defer);
  }

  /**
   * @brief Updates the headline and/or page content for the current project.
   * Both fields are locale-keyed, so you can set content for multiple languages
   * in a single request.
   *
   * @param headline A map of locales to headline strings. Each headline must be
   * between 3 and 140 characters.
   * @param page_content A map of locales to page content strings (Markdown
   * supported.) Each entry must be between 300 and 50,000 characters.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throw topgg::exception The headline or page_content length does not match
   * the required thresholds.
   * @since 2.0.0
   */
  inline void edit_project(const locale_map& headline,
                           const locale_map& page_content,
                           const empty_callback& callback,
                           const bool defer = false) {
    base_client::edit_project("@me", headline, page_content, callback, defer);
  }

  /**
   * @brief Creates a new announcement for the current project. Announcements
   * appear on a project’s page and can be used to notify users about updates,
   * new features, or other news.
   *
   * @param title The announcement's title.
   * @param content The announcement's body.
   * @param category The category to publish the announcement under.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throw topgg::exception The title or content length does not match the
   * required thresholds.
   * @since 2.0.0
   */
  inline void post_announcement(const std::string& title,
                                const std::string& content,
                                const announcement_category category,
                                const empty_callback& callback,
                                const bool defer = false) {
    base_client::post_announcement("@me", title, content, category, callback,
                                   defer);
  }

  /**
   * @brief Submits a single metrics payload for the current project. Use this
   * to push fresh numbers after an event such as joining or leaving a guild or
   * a player connecting.
   *
   * @param metrics The metrics payload.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  inline void post_metrics(const metrics& metrics,
                           const empty_callback& callback,
                           const bool defer = false) {
    base_client::post_metrics("@me", metrics, callback, defer);
  }

  /**
   * @brief Submits up to 100 metrics entries in a single request.
   *
   * @param metrics_ The metrics payloads.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throws topgg::exception The metrics entries length does not match the
   * required threshold.
   * @since 2.0.0
   */
  template <class T>
  inline void post_metrics(const timestamped_metrics<T>& metrics_,
                           const empty_callback& callback,
                           const bool defer = false) {
    base_client::post_metrics("@me", metrics_, callback, defer);
  }

  /**
   * @brief Overwrites the list of slash command definitions for the current bot
   * project on Top.gg. This is only applicable to bot-type projects on the
   * discord platform.
   *
   * @param commands The array of slash commands in a JSON-stringified Discord
   * API's application command structure format.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  inline void post_commands(const std::string& commands,
                            const empty_callback& callback,
                            const bool defer = false) {
    base_client::post_commands("@me", commands, callback, defer);
  }

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project,
   * ordered by creation date (oldest first within each page.)
   *
   * @param since The creation date frame of reference.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @throws topgg::exception The creation date frame of reference is invalid.
   * @since 2.0.0
   */
  inline void get_votes(const time_t since,
                        const paginated_callback<vote>& callback,
                        const bool defer = false) {
    base_client::get_votes("@me", since, callback, defer);
  }

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project
   * from a cursor.
   *
   * @param cursor The cursor frame of reference.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  inline void get_votes(const paginated_result<vote>& cursor,
                        const paginated_callback<vote>& callback,
                        const bool defer = false) {
    base_client::get_votes("@me", cursor, callback, defer);
  }

  /**
   * @brief Fetches the most recent vote status for a specific user. Use this to
   * check whether a user has voted before granting in-app rewards or unlocking
   * features.
   *
   * @param user_id The ID of the user to look up. The expected format depends
   * on the source parameter.
   * @param source The ID type being provided.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  inline void get_votes(const std::string& user_id, const user_source& source,
                        const callback<partial_vote>& callback,
                        const bool defer = false) {
    base_client::get_votes("@me", user_id, source, callback, defer);
  }
};
#endif

#ifndef TOPGG_PROJECT_TOKENS_ONLY
/**
 * @brief An oauth2 session.
 *
 * @since 2.0.0
 */
struct oauth2_session {
  /**
   * @brief The session's access token.
   *
   * @since 2.0.0
   */
  std::string token{};

  /**
   * @brief The session's refresh token.
   *
   * @since 2.0.0
   */
  std::string refresh_token{};

  /**
   * @brief When the session's access token expires.
   *
   * @since 2.0.0
   */
  time_t token_expires_at{};

  /**
   * @brief Whether the session's access token has expired.
   *
   * @return bool Whether the session's access token has expired.
   * @since 2.0.0
   */
  inline bool has_expired() const noexcept {
    return time(nullptr) >= token_expires_at;
  }
};

class oauth2;

/**
 * @brief An oauth2 URL.
 *
 * @since 2.0.0
 */
class oauth2_url {
  oauth2* m_oauth2{nullptr};

  inline oauth2_url(oauth2* oauth2_, const std::string& state_,
                    const std::string& code_verifier_,
                    const std::string& url_)
      : m_oauth2(oauth2_), state(state_), code_verifier(code_verifier_), url(url_) {}

 public:
  oauth2_url() = delete;

  /**
   * @brief The state.
   *
   * @since 2.0.0
   */
  std::string state{};

  /**
   * @brief The code verifier.
   *
   * @since 2.0.0
   */
  std::string code_verifier{};

  /**
   * @brief The URL.
   *
   * @since 2.0.0
   */
  std::string url{};

  /**
   * @brief Exchanges an authorization code for an oauth2 session.
   *
   * @param code The request's code query parameter.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void exchange(const std::string& code,
                const callback<oauth2_session>& callback);

  friend class oauth2;
};

class oauth2_client;

/**
 * @brief A thread-safe oauth2 manager.
 *
 * @since 2.0.0
 */
class oauth2 {
  http_frontend m_http{};
  std::string m_client_id{};
  std::string m_client_secret{};
  std::string m_scopes{};
  std::string m_redirect_uri{};
  std::mutex m_url_mutex{};
  EVP_MD* m_md{nullptr};
  EVP_MD_CTX* m_md_context{nullptr};
  BIO* m_bio{nullptr};
  BIO* m_base64_bio{nullptr};

 public:
  oauth2() = delete;

  /**
   * @brief Creates an oauth2 manager instance.
   *
   * @param client_id The client ID.
   * @param client_secret The client secret.
   * @param redirect_uri The redirect URI.
   * @param scopes An array of scope strings.
   * @throws topgg::exception Unable to create OpenSSL objects.
   * @since 2.0.0
   */
  oauth2(const std::string& client_id, const std::string& client_secret,
         const std::string& redirect_uri,
         const std::initializer_list<std::string_view>& scopes);

  /**
   * @brief Creates an oauth2 API client from an oauth2 session.
   *
   * @param session The session.
   * @return std::shared_ptr<topgg::oauth2_client> The oauth2 API client.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_client> new_client(const oauth2_session& session);

  /**
   * @brief Creates an oauth2 URL from an existing state and code verifier.
   *
   * @param state The existing state.
   * @param code_verifier The existing code verifier.
   * @return std::shared_ptr<topgg::oauth2_url> The oauth2 URL.
   * @throws topgg::exception Code verifier computation failure.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_url> new_url(const std::string& state, const std::string& code_verifier);

  /**
   * @brief Creates an oauth2 URL.
   *
   * @return std::shared_ptr<topgg::oauth2_url> The oauth2 URL.
   * @throws topgg::exception Code verifier computation failure.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_url> new_url();

  ~oauth2();

  friend class oauth2_url;
  friend class oauth2_client;
};

/**
 * @brief An oauth2 API client.
 *
 * @since 2.0.0
 */
class oauth2_client : public base_client,
                      private std::enable_shared_from_this<oauth2_client> {
  oauth2* m_oauth2{nullptr};
  oauth2_session m_session{};
  std::mutex m_token_mutex{};
  uv_timer_t m_oauth2_refresh_timer{};

  inline oauth2_client(oauth2* oauth2, const oauth2_session& session)
      : m_oauth2(oauth2), m_session(session) {
    m_oauth2_refresh_timer.data = nullptr;
  }

  inline http_frontend* get_http() override { return &m_oauth2->m_http; }

  std::string get_token() override;

  void refresh_token();

  void stop_refresh_token();

 public:
  oauth2_client() = delete;

  /**
   * @brief Retrieves the client's oauth2 session.
   *
   * @return topgg::oauth2_session The oauth2 session.
   * @since 2.0.0
   */
  oauth2_session get_session();

  /**
   * @brief Whether the client's session access token has expired.
   *
   * @return bool Whether the client's session access token has expired.
   * @since 2.0.0
   */
  bool has_expired();

  /**
   * @brief Fetches the user who authorized the application.
   *
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_authorized_user(const callback<user>& callback,
                           const bool defer = false);

  /**
   * @brief Fetches the projects owned by the user who authorized the
   * application.
   *
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void get_authorized_user_projects(
      const callback<std::vector<user_project>>& callback,
      const bool defer = false);

  /**
   * @brief Creates a draft project for the user who authorized the application.
   * Name, icon, and missing descriptions are fetched from the platform. The
   * project starts as a draft; the user completes the listing and submits it
   * for review in their Top.gg dashboard.
   *
   * @param submission The project submission.
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void submit_project(const project_submission& submission,
                      const empty_callback& callback, const bool defer = false);

  /**
   * @brief Revokes the client's access token.
   *
   * @param callback The API call's callback function.
   * @param defer Whether to defer the request later. Defaults to false.
   * @since 2.0.0
   */
  void revoke_token(const empty_callback& callback, const bool defer = false);

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
  ~oauth2_client();
#endif

  friend class http_backend;
  friend class oauth2;
};
#endif
};  // namespace topgg