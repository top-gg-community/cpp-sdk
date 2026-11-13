#ifndef NGHTTP2_NO_SSIZE_T
#define NGHTTP2_NO_SSIZE_T
#endif

#include <nghttp2/nghttp2.h>
#include <openssl/err.h>
#include <topgg/topgg.h>
#include <uv.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#endif

topgg::exception topgg::exception::ssl(const char* message) {
  topgg::exception exc{message};

  unsigned long error{};

  while ((error = ERR_get_error()) != 0) {
    exc.cause += ERR_error_string(error, nullptr);
    exc.cause += "\n";
  }

  TOPGG_ELOGF("SSL ERROR: %s - %s", exc.what(), exc.cause.c_str());

  return exc;
}

topgg::exception topgg::exception::uv(const char* message, const int status) {
  topgg::exception exc{message};

  exc.cause = const_cast<char*>(uv_err_name(status));

  TOPGG_ELOGF("UV ERROR: %s - %s", exc.what(), exc.cause.c_str());

  return exc;
}

#ifdef _WIN32
topgg::exception topgg::exception::system(const char* message) {
  topgg::exception exc{message};

  auto error{GetLastError()};

  if (error == 0) {
    error = WSAGetLastError();

    WSASetLastError(0);
  } else {
    SetLastError(0);
  }

  if (error != 0) {
    LPSTR cause_buf{nullptr};

    const auto size{FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&cause_buf), 0, nullptr)};

    if (size > 0) {
      exc.cause = std::string{cause_buf, size};

      LocalFree(cause_buf);
    }
  }

  TOPGG_ELOGF("SYSTEM ERROR: %s - %s", exc.what(), exc.cause.c_str());

  return exc;
}
#endif

topgg::exception topgg::exception::nghttp2(const char* message,
                                           const int status) {
  topgg::exception exc{message};

  exc.cause = const_cast<char*>(nghttp2_strerror(status));

  TOPGG_ELOGF("NGHTTP2 ERROR: %s - %s", exc.what(), exc.cause.c_str());

  return exc;
}

topgg::http_exception::http_exception(const topgg::http_response& response)
    : topgg::exception("Got an invalid HTTP status code from Top.gg"),
      status(response.status),
      retry_after(response.retry_after) {
  try {
    const auto j{nlohmann::json::parse(response.body)};

    cause = j.value(j.contains("error") ? "error" : "title", "");
    detail = j.value(
        j.contains("error_description") ? "error_description" : "detail",
        cause);
  } catch (const nlohmann::json::exception&) {
  }

  TOPGG_ELOGF("HTTP ERROR: %d", response.status);
}