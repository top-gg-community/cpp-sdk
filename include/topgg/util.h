#pragma once

#include <topgg/topgg.h>

#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

namespace topgg {
class waker {
#ifdef _WIN32
  HANDLE m_semaphore{nullptr};
#endif

  waker();

  void notify();

  void wait();

  ~waker();

  friend class http_frontend;
  friend class http_backend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  friend class oauth2;
  friend class oauth2_client;
#endif
};

#ifdef _WIN32
class wsastartup_guard {
  void init();

  ~wsastartup_guard();

  friend class http_backend;
};
#endif

platform _from_platform_string(const std::string& platform);

project_type _from_project_type_string(const std::string& type);

time_t _from_time_string(const std::string& timestamp);

std::string _to_time_string(const time_t timestamp);

std::string _url_encode(const std::string& text);

#ifndef TOPGG_PROJECT_TOKENS_ONLY
std::string _random_string();
#endif
};  // namespace topgg