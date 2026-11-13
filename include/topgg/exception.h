#pragma once

#include <topgg/topgg.h>

#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>

namespace topgg {
/**
 * @brief A standard exception.
 *
 * @since 2.0.0
 */
class exception : public std::runtime_error {
 protected:
  inline exception(const char* message) : std::runtime_error(message) {}

  static exception ssl(const char* message);

  static exception uv(const char* message, const int status);

#ifdef _WIN32
  static exception system(const char* message);
#endif

  static exception nghttp2(const char* message, const int status);

 public:
  /**
   * @brief The exception's cause if any.
   *
   * @since 2.0.0
   */
  std::string cause{};

  exception() = delete;

  friend time_t _from_time_string(const std::string& timestamp);
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend std::string _random_string();
#endif
  friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  friend class client;
#endif
  friend class http_backend;
  friend class http_frontend;
  friend class locale_map;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
  friend class oauth2;
#endif
  friend class waker;
#ifdef TOPGG_WEBHOOKS
  friend webhooks::payload webhooks::parse(const std::string& body);
  friend class webhooks::verifier;
#endif
#ifdef _WIN32
  friend class wsastartup_guard;
#endif
};

struct http_response;

/**
 * @brief An HTTP request exception.
 *
 * @since 2.0.0
 */
class http_exception : public exception {
  http_exception(const http_response& response);

 public:
  /**
   * @brief The exception's HTTP status code.
   *
   * @since 2.0.0
   */
  uint16_t status{};

  /**
   * @brief How many seconds to wait before a ratelimit is lifted.
   *
   * @since 2.0.0
   */
  std::optional<uint32_t> retry_after{};

  /**
   * @brief The exception's detail if any.
   *
   * @since 2.0.0
   */
  std::string detail{};

  http_exception() = delete;

  friend class base_client;
  friend class http_backend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2_client;
  friend class oauth2_url;
  friend class oauth2;
#endif
};
};  // namespace topgg