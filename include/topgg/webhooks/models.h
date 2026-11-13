#pragma once

#include <topgg/topgg.h>

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>

/**
 * @brief The Top.gg namespace for webhooks support.
 *
 * @since 2.0.0
 */
namespace topgg::webhooks {
class vote_create;
class test;
class integration_create;
class integration_delete;

/**
 * @brief A parsed webhook payload.
 *
 * @since 2.0.0
 */
using payload =
    std::variant<vote_create, test, integration_create, integration_delete>;

/**
 * @brief A project.
 *
 * @since 2.0.0
 */
class project {
  project(const nlohmann::json& j);

 public:
  project() = delete;

  /**
   * @brief The project's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  /**
   * @brief The project's	Discord client ID or guild ID.
   *
   * @since 2.0.0
   */
  std::string platform_id{};

  /**
   * @brief The project's platform (always topgg::platform::p_discord).
   *
   * @since 2.0.0
   */
  topgg::platform platform{};

  /**
   * @brief The project's type (either topgg::project_type::pt_bot or
   * topgg::project_type::pt_server).
   *
   * @since 2.0.0
   */
  project_type type{};

  friend class vote_create;
  friend class test;
  friend class integration_create;
};

/**
 * @brief A Top.gg user.
 *
 * @since 2.0.0
 */
class user {
  user(const nlohmann::json& j);

 public:
  user() = delete;

  /**
   * @brief The user's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  /**
   * @brief The user's Discord ID.
   *
   * @since 2.0.0
   */
  std::string platform_id{};

  /**
   * @brief The user's Discord username.
   *
   * @since 2.0.0
   */
  std::string name{};

  /**
   * @brief The user's avatar URL.
   *
   * @since 2.0.0
   */
  std::string avatar{};

  friend class vote_create;
  friend class test;
  friend class integration_create;
};

/**
 * @brief A vote.create webhook payload. Top.gg fires this event each time a
 * user upvotes your project. Use it to reward voters, update leaderboards, or
 * log activity.
 *
 * @since 2.0.0
 */
class vote_create {
  vote_create(const nlohmann::json& j);

 public:
  vote_create() = delete;

  /**
   * @brief The vote's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  /**
   * @brief The vote's weight (1 normally, 2 during weekend multiplier.)
   *
   * @since 2.0.0
   */
  uint16_t weight{};

  /**
   * @brief When the vote was cast.
   *
   * @since 2.0.0
   */
  time_t created_at{};

  /**
   * @brief When the user can vote again.
   *
   * @since 2.0.0
   */
  time_t expires_at{};

  /**
   * @brief The voted project.
   *
   * @since 2.0.0
   */
  webhooks::project project;

  /**
   * @brief The voter.
   *
   * @since 2.0.0
   */
  webhooks::user user;

  /**
   * @brief The parsed query parameters appended to the /:id/vote page URL.
   *
   * @since 2.0.0
   */
  std::unordered_map<std::string, std::string> query{};

  /**
   * @brief Whether the vote has expired.
   *
   * @return bool Whether the vote has expired.
   * @since 2.0.0
   */
  inline bool has_expired() const noexcept {
    return time(nullptr) >= expires_at;
  }

  friend webhooks::payload parse(const std::string& body);
};

/**
 * @brief A webhook.test webhook payload. This event is always available and
 * lets you verify your endpoint is reachable before real votes arrive. Send a
 * test from your project’s Webhooks page in the dashboard.
 *
 * @since 2.0.0
 */
class test {
  test(const nlohmann::json& j);

 public:
  test() = delete;

  /**
   * @brief The tested project.
   *
   * @since 2.0.0
   */
  webhooks::project project;

  /**
   * @brief The tester.
   *
   * @since 2.0.0
   */
  webhooks::user user;

  friend webhooks::payload parse(const std::string& body);
};

/**
 * @brief A integration.create webhook payload that fires when a user visits
 * your integration page on Top.gg and clicks the Connect button.
 *
 * @since 2.0.0
 */
class integration_create {
  integration_create(const nlohmann::json& j);

 public:
  integration_create() = delete;

  /**
   * @brief The connection's Top.gg ID. Store this to track and delete
   * connections.
   *
   * @since 2.0.0
   */
  std::string connection_id{};

  /**
   * @brief The webhook secret (prefixed whs_) used to verify all future webhook
   * deliveries for this connection.
   *
   * @since 2.0.0
   */
  std::string webhook_secret{};

  /**
   * @brief The connection's project.
   *
   * @since 2.0.0
   */
  webhooks::project project;

  /**
   * @brief The connection's user.
   *
   * @since 2.0.0
   */
  webhooks::user user;

  friend webhooks::payload parse(const std::string& body);
};

/**
 * @brief A integration.delete webhook payload that fires when a user removes
 * the managed webhook from their dashboard.
 *
 * @since 2.0.0
 */
class integration_delete {
  integration_delete(const nlohmann::json& j);

 public:
  integration_delete() = delete;

  /**
   * @brief The connection's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string connection_id{};

  friend webhooks::payload parse(const std::string& body);
};
};  // namespace topgg::webhooks