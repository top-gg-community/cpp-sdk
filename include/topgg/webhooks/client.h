#pragma once

#include <openssl/evp.h>
#include <openssl/params.h>
#include <topgg/topgg.h>

#include <mutex>
#include <string>

namespace topgg::webhooks {
/**
 * @brief Parses a webhook request's JSON body.
 *
 * @param body The webhook request's JSON body.
 * @return topgg::payload The parsed webhook payload.
 * @throws topgg::exception The webhook payload's event type is unrecognized.
 * @throws nlohmann::json::exception Unable to parse the JSON body.
 * @since 2.0.0
 */
payload parse(const std::string& body);

/**
 * @brief A thread-safe helper that helps verify incoming webhook requests.
 *
 * @since 2.0.0
 */
class verifier {
  std::mutex m_mutex{};
  EVP_MAC* m_mac{};
  EVP_MAC_CTX* m_mac_context{};
  OSSL_PARAM m_params[2];

 public:
  /**
   * @brief Creates a verifier instance.
   *
   * @throws topgg::exception Unable to create HMAC context.
   * @since 2.0.0
   */
  verifier();

  /**
   * @brief Verifies an incoming webhook request.
   *
   * @param secret The webhook's current secret.
   * @param signature The request's x-topgg-signature header.
   * @param body The request's raw body.
   * @return bool Whether the request is authorized.
   * @since 2.0.0
   */
  bool verify(const std::string& secret, const std::string& signature,
              const std::string& body);

  ~verifier();
};
};  // namespace topgg::webhooks