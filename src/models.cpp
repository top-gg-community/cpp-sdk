#include <topgg/topgg.h>

topgg::partial_project::partial_project(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  platform_id = j["platform_id"].template get<std::string>();
  name = j["name"].template get<std::string>();

  platform =
      topgg::_from_platform_string(j["platform"].template get<std::string>());
  type =
      topgg::_from_project_type_string(j["type"].template get<std::string>());
}

topgg::project::project(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  name = j["name"].template get<std::string>();
  platform =
      topgg::_from_platform_string(j["platform"].template get<std::string>());
  type =
      topgg::_from_project_type_string(j["type"].template get<std::string>());
  headline = j["headline"].template get<std::string>();

  for (const auto& tag : j["tags"]) {
    tags.push_back(tag.template get<std::string>());
  }

  current_votes = j["votes"].template get<uint64_t>();
  total_votes = j["votes_total"].template get<uint64_t>();
  review_score = j["review_score"].template get<float>();
  review_count = j["review_count"].template get<uint64_t>();
}

void topgg::locale_map::enforce_constraints(const uint16_t minimum,
                                            const uint16_t maximum) const {
  for (const auto& item : m_json.items()) {
    const auto text{item.value().template get<std::string_view>()};

    if (text.length() < minimum) {
      throw topgg::exception{
          "A locale_map text length is below the minimum threshold"};
    } else if (text.length() > maximum) {
      throw topgg::exception{
          "A locale_map text length is above the maximum threshold"};
    }
  }
}

void topgg::locale_map::set(const topgg::locale& locale,
                            const std::string& text) {
  std::string key{};

  switch (locale) {
    case topgg::locale::l_english:
      key = "en";
      break;
    case topgg::locale::l_german:
      key = "de";
      break;
    case topgg::locale::l_french:
      key = "fr";
      break;
    case topgg::locale::l_portuguese:
      key = "pt";
      break;
    case topgg::locale::l_turkish:
      key = "tr";
      break;
    case topgg::locale::l_hindi:
      key = "hi";
      break;
    case topgg::locale::l_japanese:
      key = "ja";
      break;
    case topgg::locale::l_arabic:
      key = "ar";
      break;
    case topgg::locale::l_dutch:
      key = "nl";
      break;
    case topgg::locale::l_korean:
      key = "ko";
      break;
    case topgg::locale::l_italian:
      key = "it";
      break;
    case topgg::locale::l_spanish:
      key = "es";
      break;
    case topgg::locale::l_russian:
      key = "ru";
      break;
    case topgg::locale::l_ukrainian:
      key = "uk";
      break;
    case topgg::locale::l_vietnamese:
      key = "vi";
      break;
    case topgg::locale::l_chinese_simplified:
      key = "zh";
      break;
  }

  m_json[key] = text;
}

nlohmann::json topgg::discord_bot_metrics::to_json() const {
  nlohmann::json json{};

  if (m_server_count.has_value()) {
    json["server_count"] = m_server_count.value();
  }

  if (m_shard_count.has_value()) {
    json["shard_count"] = m_shard_count.value();
  }

  return json;
}

nlohmann::json topgg::discord_server_metrics::to_json() const {
  nlohmann::json json{};

  if (m_total_member_count.has_value()) {
    json["member_count"] = m_total_member_count.value();
  }

  if (m_online_member_count.has_value()) {
    json["online_count"] = m_online_member_count.value();
  }

  return json;
}

nlohmann::json topgg::roblox_metrics::to_json() const {
  nlohmann::json json{};

  json["player_count"] = m_player_count;

  return json;
}

topgg::partial_vote::partial_vote(const nlohmann::json& j) {
  weight = j["weight"].template get<uint16_t>();
  created_at =
      topgg::_from_time_string(j["created_at"].template get<std::string>());
  expires_at =
      topgg::_from_time_string(j["expires_at"].template get<std::string>());
}

topgg::vote::vote(const nlohmann::json& j) : topgg::partial_vote(j) {
  user_id = j["user_id"].template get<std::string>();
  platform_id = j["platform_id"].template get<std::string>();
}

#ifndef TOPGG_PROJECT_TOKENS_ONLY
topgg::integration::integration(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  name = j["name"].template get<std::string>();
  description = j["description"].template get<std::string>();
  icon_url = j["icon_url"].template get<std::string>();
  connected = j["connected"].template get<bool>();
}

topgg::base_webhook::base_webhook(const nlohmann::json& j) {
  label = j["label"].template get<std::string>();
  url = j["url"].template get<std::string>();
}

topgg::webhook::webhook(const nlohmann::json& j) : topgg::base_webhook(j) {
  id = j["id"].template get<std::string>();
}

topgg::user::user(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  username = j["username"].template get<std::string>();

  if (j.contains("avatar")) {
    avatar = j["avatar"].template get<std::optional<std::string>>();
  }

  for (const auto& connection : j["connections"]) {
    connections.push_back(
        {topgg::_from_platform_string(
             connection["platform"].template get<std::string>()),
         connection["id"].template get<std::string>()});
  }
}

topgg::user_project::user_project(const nlohmann::json& j)
    : topgg::partial_project(j) {
  headline = j["headline"].template get<std::string>();

  const auto review_status_{j["review_status"].template get<std::string>()};

  if (review_status_ == "draft") {
    review_status = topgg::review_status::rs_draft;
  } else if (review_status_ == "in_review") {
    review_status = topgg::review_status::rs_in_review;
  } else {
    review_status = topgg::review_status::rs_approved;
  }
}
#endif

#ifdef TOPGG_WEBHOOKS
topgg::webhooks::project::project(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  platform_id = j["platform_id"].template get<std::string>();
  platform =
      topgg::_from_platform_string(j["platform"].template get<std::string>());
  type =
      topgg::_from_project_type_string(j["type"].template get<std::string>());
}

topgg::webhooks::user::user(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  platform_id = j["platform_id"].template get<std::string>();
  name = j["name"].template get<std::string>();
  avatar = j["avatar_url"].template get<std::string>();
}

topgg::webhooks::vote_create::vote_create(const nlohmann::json& j)
    : project(j["project"]), user(j["user"]) {
  id = j["id"].template get<std::string>();
  weight = j["weight"].template get<uint16_t>();
  created_at = topgg::_from_time_string(
      j["created_at"].template get<std::string>().substr(0, 19));
  expires_at = topgg::_from_time_string(
      j["expires_at"].template get<std::string>().substr(0, 19));
  query =
      j["query"].template get<std::unordered_map<std::string, std::string>>();
}

topgg::webhooks::test::test(const nlohmann::json& j)
    : project(j["project"]), user(j["user"]) {}

topgg::webhooks::integration_create::integration_create(const nlohmann::json& j)
    : project(j["project"]), user(j["user"]) {
  connection_id = j["connection_id"].template get<std::string>();
  webhook_secret = j["webhook_secret"].template get<std::string>();
}

topgg::webhooks::integration_delete::integration_delete(
    const nlohmann::json& j) {
  connection_id = j["connection_id"].template get<std::string>();
}
#endif