#include <topgg/topgg.h>

#include <sstream>

#ifndef TOPGG_PROJECT_TOKENS_ONLY
#include <openssl/rand.h>
#endif

#ifdef _WIN32
#include <winsock2.h>

static thread_local size_t g_wsastartup_counter = 0;
#endif

topgg::waker::waker() {
#ifdef _WIN32
  if ((m_semaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr)) == nullptr) {
    throw topgg::exception::system("Unable to create semaphore");
  }
#endif
}

void topgg::waker::notify() {
#ifdef _WIN32
  if (ReleaseSemaphore(m_semaphore, 1, nullptr) == 0 &&
      GetLastError() != ERROR_TOO_MANY_POSTS) {
    throw topgg::exception::system("Unable to notify semaphore");
  }
#endif
}

void topgg::waker::wait() {
#ifdef _WIN32
  if (WaitForSingleObject(m_semaphore, INFINITE) == WAIT_FAILED) {
    throw topgg::exception::system("Unable to wait for semaphore");
  }
#endif
}

topgg::waker::~waker() {
#ifdef _WIN32
  if (m_semaphore != nullptr) {
    CloseHandle(m_semaphore);
  }
#endif
}

#ifdef _WIN32
void topgg::wsastartup_guard::init() {
  if (g_wsastartup_counter == 0) {
    WSADATA wsa_data{};

    if (const auto status{WSAStartup(MAKEWORD(2, 2), &wsa_data)}; status != 0) {
      throw topgg::exception::system("Unable to call WSAStartup");
    }

    g_wsastartup_counter++;
  }
}

topgg::wsastartup_guard::~wsastartup_guard() {
  if (g_wsastartup_counter > 0 && g_wsastartup_counter-- == 1) {
    WSACleanup();
  }
}
#endif

topgg::platform topgg::_from_platform_string(const std::string& platform) {
  if (platform == "discord") {
    return topgg::platform::p_discord;
  }

  return topgg::platform::p_roblox;
}

topgg::project_type topgg::_from_project_type_string(const std::string& type) {
  if (type == "bot") {
    return topgg::project_type::pt_bot;
  } else if (type == "server") {
    return topgg::project_type::pt_server;
  }

  return topgg::project_type::pt_game;
}

time_t topgg::_from_time_string(const std::string& timestamp) {
  std::tm tm{};
  std::istringstream ss{timestamp};

  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail()) {
    throw topgg::exception{"Unable to parse ISO timestamp"};
  }

#ifdef _WIN32
  return _mkgmtime(&tm);
#else
  return timegm(&tm);
#endif
}

std::string topgg::_to_time_string(const time_t timestamp) {
  std::ostringstream ss{};

  ss << std::put_time(std::gmtime(&timestamp), "%FT%TZ");

  return ss.str();
}

std::string topgg::_url_encode(const std::string& text) {
  std::ostringstream ss{};

  ss << std::hex << std::uppercase;

  for (const auto c : text) {
    if (std::isalnum(static_cast<uint8_t>(c)) || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      ss << c;
    } else if (c == ' ') {
      ss << '+';
    } else {
      ss << '%' << std::setw(2) << std::setfill('0')
         << static_cast<int>(static_cast<uint8_t>(c));
    }
  }

  return ss.str();
}

#ifndef TOPGG_PROJECT_TOKENS_ONLY
std::string topgg::_random_string() {
  std::vector<uint8_t> buf(64);

  if (RAND_bytes(buf.data(), 64) != 1) {
    throw topgg::exception{"Unable to generate randomized string"};
  }

  std::stringstream ss{};

  ss << std::hex << std::setfill('0');

  for (const auto c : buf) {
    ss << std::setw(2) << static_cast<int>(c);
  }

  return ss.str();
}
#endif