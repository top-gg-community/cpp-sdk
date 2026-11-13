#pragma once

#include <ctime>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

/**
 * @brief The Top.gg namespace.
 *
 * @since 2.0.0
 */
namespace topgg {
template <class T>
class paginated_result;

/**
 * @brief A project's platform.
 *
 * @since 2.0.0
 */
enum platform {
  /**
   * @brief Discord.
   *
   * @since 2.0.0
   */
  p_discord,

  /**
   * @brief Roblox.
   *
   * @since 2.0.0
   */
  p_roblox,
};

/**
 * @brief A project's type.
 *
 * @since 2.0.0
 */
enum project_type {
  /**
   * @brief A bot.
   *
   * @since 2.0.0
   */
  pt_bot,

  /**
   * @brief A server.
   *
   * @since 2.0.0
   */
  pt_server,

  /**
   * @brief A game.
   *
   * @since 2.0.0
   */
  pt_game,
};

/**
 * @brief A partial project.
 *
 * @since 2.0.0
 */
class partial_project {
 protected:
  partial_project(const nlohmann::json& j);

 public:
  partial_project() = delete;

  /**
   * @brief The project's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  /**
   * @brief The project's platform ID.
   *
   * @since 2.0.0
   */
  std::string platform_id{};

  /**
   * @brief The project's name in its platform.
   *
   * @since 2.0.0
   */
  std::string name{};

  /**
   * @brief The project's platform.
   *
   * @since 2.0.0
   */
  topgg::platform platform{};

  /**
   * @brief The project's type.
   *
   * @since 2.0.0
   */
  project_type type{};

  friend class base_client;
  friend class paginated_result<partial_project>;
};

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
   * @brief The project's name in its platform.
   *
   * @since 2.0.0
   */
  std::string name{};

  /**
   * @brief The project's platform.
   *
   * @since 2.0.0
   */
  topgg::platform platform{};

  /**
   * @brief The project's type.
   *
   * @since 2.0.0
   */
  project_type type{};

  /**
   * @brief The project's headline.
   *
   * @since 2.0.0
   */
  std::string headline{};

  /**
   * @brief The project's tags.
   *
   * @since 2.0.0
   */
  std::vector<std::string> tags{};

  /**
   * @brief The project's current vote count.
   *
   * @since 2.0.0
   */
  uint64_t current_votes{};

  /**
   * @brief The project's vote count.
   *
   * @since 2.0.0
   */
  uint64_t total_votes{};

  /**
   * @brief The project's review score.
   *
   * @since 2.0.0
   */
  float review_score{};

  /**
   * @brief The project's review count.
   *
   * @since 2.0.0
   */
  uint64_t review_count{};

  friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
  friend class paginated_result<project>;
};

/**
 * @brief A supported locale.
 *
 * @since 2.0.0
 */
enum locale {
  /**
   * @brief English.
   *
   * @since 2.0.0
   */
  l_english,

  /**
   * @brief German.
   *
   * @since 2.0.0
   */
  l_german,

  /**
   * @brief French.
   *
   * @since 2.0.0
   */
  l_french,

  /**
   * @brief Portuguese.
   *
   * @since 2.0.0
   */
  l_portuguese,

  /**
   * @brief Turkish.
   *
   * @since 2.0.0
   */
  l_turkish,

  /**
   * @brief Hindi.
   *
   * @since 2.0.0
   */
  l_hindi,

  /**
   * @brief Japanese.
   *
   * @since 2.0.0
   */
  l_japanese,

  /**
   * @brief Arabic.
   *
   * @since 2.0.0
   */
  l_arabic,

  /**
   * @brief Dutch.
   *
   * @since 2.0.0
   */
  l_dutch,

  /**
   * @brief Korean.
   *
   * @since 2.0.0
   */
  l_korean,

  /**
   * @brief Italian.
   *
   * @since 2.0.0
   */
  l_italian,

  /**
   * @brief Spanish.
   *
   * @since 2.0.0
   */
  l_spanish,

  /**
   * @brief Russian.
   *
   * @since 2.0.0
   */
  l_russian,

  /**
   * @brief Ukrainian.
   *
   * @since 2.0.0
   */
  l_ukrainian,

  /**
   * @brief Vietnamese.
   *
   * @since 2.0.0
   */
  l_vietnamese,

  /**
   * @brief Chinese Simplified.
   *
   * @since 2.0.0
   */
  l_chinese_simplified,
};

/**
 * @brief A locale-text map.
 *
 * @since 2.0.0
 */
class locale_map {
  nlohmann::json m_json{};

  void enforce_constraints(const uint16_t minimum,
                           const uint16_t maximum) const;

 public:
  /**
   * @brief Inserts a new locale-text entry.
   *
   * @param locale_ The locale.
   * @param text The text.
   * @since 2.0.0
   */
  void set(const locale& locale_, const std::string& text);

  friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  friend class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
};

/**
 * @brief An announcement category.
 *
 * @since 2.0.0
 */
enum announcement_category {
  /**
   * @brief A generic announcement.
   *
   * @since 2.0.0
   */
  ac_announcement,

  /**
   * @brief An event.
   *
   * @since 2.0.0
   */
  ac_event,

  /**
   * @brief A new feature.
   *
   * @since 2.0.0
   */
  ac_new_feature,
};

/**
 * @brief A project's metrics.
 *
 * @since 2.0.0
 */
class metrics {
  virtual nlohmann::json to_json() const = 0;

  friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  friend class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
};

template <class T>
class timestamped_metrics;

/**
 * @brief A Discord bot's metrics.
 *
 * @since 2.0.0
 */
class discord_bot_metrics : public metrics {
  std::optional<uint64_t> m_server_count{};
  std::optional<uint64_t> m_shard_count{};

  inline constexpr discord_bot_metrics(
      const std::pair<std::optional<uint64_t>, std::optional<uint64_t>>& pair)
      : m_server_count(pair.first), m_shard_count(pair.second) {}

  nlohmann::json to_json() const override;

 public:
  discord_bot_metrics() = delete;

  /**
   * @brief Creates a Discord bot's metrics instance.
   *
   * @param server_count_ The bot's current server count.
   * @param shard_count_ The bot's current shard count.
   * @since 2.0.0
   */
  inline constexpr discord_bot_metrics(const uint64_t server_count_,
                                       const uint64_t shard_count_)
      : m_server_count(server_count_), m_shard_count(shard_count_) {}

  /**
   * @brief Creates a Discord bot's metrics instance.
   *
   * @param server_count_ The bot's current server count.
   * @return topgg::discord_bot_metrics The Discord bot's metrics.
   * @since 2.0.0
   */
  static inline constexpr discord_bot_metrics server_count(
      const uint64_t server_count_) noexcept {
    return {std::make_pair(server_count_, std::nullopt)};
  }

  /**
   * @brief Creates a Discord bot's metrics instance.
   *
   * @param shard_count_ The bot's current shard count.
   * @return topgg::discord_bot_metrics The Discord bot's metrics.
   * @since 2.0.0
   */
  static inline constexpr discord_bot_metrics shard_count(
      const uint64_t shard_count_) noexcept {
    return {std::make_pair(std::nullopt, shard_count_)};
  }

  friend class timestamped_metrics<discord_bot_metrics>;
};

/**
 * @brief A Discord server's metrics.
 *
 * @since 2.0.0
 */
class discord_server_metrics : public metrics {
  std::optional<uint64_t> m_total_member_count{};
  std::optional<uint64_t> m_online_member_count{};

  inline constexpr discord_server_metrics(
      const std::pair<std::optional<uint64_t>, std::optional<uint64_t>>& pair)
      : m_total_member_count(pair.first), m_online_member_count(pair.second) {}

  nlohmann::json to_json() const override;

 public:
  discord_server_metrics() = delete;

  /**
   * @brief Creates a Discord server's metrics instance.
   *
   * @param total_member_count_ The server's current member count.
   * @param online_member_count_ The server's current online member count.
   * @since 2.0.0
   */
  inline constexpr discord_server_metrics(const uint64_t total_member_count_,
                                          const uint64_t online_member_count_)
      : m_total_member_count(total_member_count_),
        m_online_member_count(online_member_count_) {}

  /**
   * @brief Creates a Discord server's metrics instance.
   *
   * @param total_member_count_ The server's current member count.
   * @return topgg::discord_server_metrics The Discord server's metrics.
   * @since 2.0.0
   */
  static inline constexpr discord_server_metrics total_member_count(
      const uint64_t total_member_count_) noexcept {
    return {std::make_pair(total_member_count_, std::nullopt)};
  }

  /**
   * @brief Creates a Discord server's metrics instance.
   *
   * @param online_member_count_ The server's current online member count.
   * @return topgg::discord_server_metrics The Discord server's metrics.
   * @since 2.0.0
   */
  static inline constexpr discord_server_metrics online_member_count(
      const uint64_t online_member_count_) noexcept {
    return {std::make_pair(std::nullopt, online_member_count_)};
  }

  friend class timestamped_metrics<discord_server_metrics>;
};

/**
 * @brief A Roblox game's metrics.
 *
 * @since 2.0.0
 */
class roblox_metrics : public metrics {
  uint64_t m_player_count{};

  nlohmann::json to_json() const override;

 public:
  roblox_metrics() = delete;

  /**
   * @brief Creates a Roblox game's metrics instance.
   *
   * @param player_count The game's current player count.
   * @since 2.0.0
   */
  inline constexpr roblox_metrics(const uint64_t player_count)
      : m_player_count(player_count) {}

  friend class timestamped_metrics<roblox_metrics>;
};

std::string _to_time_string(const time_t timestamp);

/**
 * @brief A project's timestamped metrics.
 *
 * @since 2.0.0
 */
template <class T>
class timestamped_metrics {
  static_assert(
      std::is_base_of_v<metrics, T>,
      "timestamped_metrics class template must be a child of metrics");

  nlohmann::json m_json{};

 public:
  inline timestamped_metrics() : m_json(nlohmann::json::array()) {}

  /**
   * @brief Inserts a new timestamped metrics entry.
   *
   * @param timestamp The timestamp.
   * @param metrics_ The project's metrics.
   * @since 2.0.0
   */
  void add(const time_t timestamp, const T& metrics_) {
    nlohmann::json json{};

    json["timestamp"] = _to_time_string(timestamp);
    json["metrics"] = metrics_.to_json();

    m_json.push_back(json);
  }

  /**
   * @brief Inserts a new timestamped metrics entry using the current timestamp.
   *
   * @param metrics_ The project's metrics.
   * @since 2.0.0
   */
  inline void add(const T& metrics_) { add(time(nullptr), metrics_); }

  /**
   * @brief Clears the contents.
   *
   * @since 2.0.0
   */
  inline void clear() { m_json.clear(); }

  friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
};

/**
 * @brief A partial vote.
 *
 * @since 2.0.0
 */
class partial_vote {
 protected:
  partial_vote(const nlohmann::json& j);

 public:
  partial_vote() = delete;

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
   * @brief Whether the vote has expired.
   *
   * @return bool Whether the vote has expired.
   * @since 2.0.0
   */
  inline bool has_expired() const noexcept {
    return time(nullptr) >= expires_at;
  }

  friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
};

/**
 * @brief A user's source platform.
 *
 * @since 2.0.0
 */
enum user_source {
  /**
   * @brief The user came from Top.gg.
   *
   * @since 2.0.0
   */
  us_topgg,

  /**
   * @brief The user came from Discord.
   *
   * @since 2.0.0
   */
  us_discord,
};

/**
 * @brief A vote.
 *
 * @since 2.0.0
 */
class vote : public partial_vote {
  vote(const nlohmann::json& j);

 public:
  vote() = delete;

  /**
   * @brief The voter's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string user_id{};

  /**
   * @brief The voter's platform ID.
   *
   * @since 2.0.0
   */
  std::string platform_id{};

  friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
#endif
  friend class paginated_result<vote>;
};

/**
 * @brief An integration.
 *
 * @since 2.0.0
 */
class integration {
  integration(const nlohmann::json& j);

 public:
  integration() = delete;

  /**
   * @brief The integration's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  /**
   * @brief The integration's name.
   *
   * @since 2.0.0
   */
  std::string name{};

  /**
   * @brief The integration's description.
   *
   * @since 2.0.0
   */
  std::string description{};

  /**
   * @brief The integration's icon URL.
   *
   * @since 2.0.0
   */
  std::string icon_url{};

  /**
   * @brief Whether the integration is connected to the project.
   *
   * @since 2.0.0
   */
  bool connected{};

  friend class base_client;
};

/**
 * @brief A base webhook.
 *
 * @since 2.0.0
 */
struct base_webhook {
  /**
   * @brief The webhook's label.
   *
   * @since 2.0.0
   */
  std::string label{};

  /**
   * @brief The webhook's URL.
   *
   * @since 2.0.0
   */
  std::string url{};

  base_webhook() = default;

 protected:
  base_webhook(const nlohmann::json& j);

  friend class base_client;
};

/**
 * @brief A webhook.
 *
 * @since 2.0.0
 */
class webhook : public base_webhook {
  webhook(const nlohmann::json& j);

 public:
  webhook() = delete;

  /**
   * @brief The webhook's Top.gg ID.
   *
   * @since 2.0.0
   */
  std::string id{};

  friend class base_client;
};

#ifndef TOPGG_PROJECT_TOKENS_ONLY
/**
 * @brief A user's platform connection.
 *
 * @since 2.0.0
 */
struct user_connection {
  /**
   * @brief The connection's platform.
   *
   * @since 2.0.0
   */
  topgg::platform platform{};

  /**
   * @brief The user's platform ID.
   *
   * @since 2.0.0
   */
  std::string id{};
};

/**
 * @brief A user.
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
   * @brief The user's username.
   *
   * @since 2.0.0
   */
  std::string username{};

  /**
   * @brief The user's avatar URL.
   *
   * @since 2.0.0
   */
  std::optional<std::string> avatar{std::nullopt};

  /**
   * @brief The user's platform connections.
   *
   * @since 2.0.0
   */
  std::vector<user_connection> connections{};

  friend class base_client;
  friend class oauth2_client;
};

/**
 * @brief A project's review status.
 *
 * @since 2.0.0
 */
enum review_status {
  /**
   * @brief The project is currently a draft.
   *
   * @since 2.0.0
   */
  rs_draft,

  /**
   * @brief The project is currently in the reviewer queue.
   *
   * @since 2.0.0
   */
  rs_in_review,

  /**
   * @brief The project is approved and listed.
   *
   * @since 2.0.0
   */
  rs_approved,
};

/**
 * @brief A user's project.
 *
 * @since 2.0.0
 */
class user_project : public partial_project {
  user_project(const nlohmann::json& j);

 public:
  user_project() = delete;

  /**
   * @brief The project's headline.
   *
   * @since 2.0.0
   */
  std::string headline{};

  /**
   * @brief The project's review status.
   *
   * @since 2.0.0
   */
  topgg::review_status review_status{};

  friend class base_client;
  friend class oauth2_client;
};

/**
 * @brief A project submission.
 *
 * @since 2.0.0
 */
struct project_submission {
  project_submission() = delete;

  inline project_submission(const platform platform_, const project_type type_,
                            const std::string& platform_id_,
                            const std::string& headline_,
                            const std::string& page_content_)
      : platform(platform_),
        type(type_),
        platform_id(platform_id_),
        headline(headline_),
        page_content(page_content_) {}

  /**
   * @brief The project's platform.
   *
   * @since 2.0.0
   */
  topgg::platform platform{};

  /**
   * @brief The project's type.
   *
   * @since 2.0.0
   */
  project_type type{};

  /**
   * @brief The project's platform ID.
   *
   * @since 2.0.0
   */
  std::string platform_id{};

  /**
   * @brief The project's headline.
   *
   * @since 2.0.0
   */
  std::string headline{};

  /**
   * @brief The project's page content.
   *
   * @since 2.0.0
   */
  std::string page_content{};
};
#endif
};  // namespace topgg