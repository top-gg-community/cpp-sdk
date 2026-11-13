#include <topgg/topgg.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

static std::mutex g_log_mutex{};

void topgg::_log(const char* file, const size_t line, const char* text) {
  std::lock_guard guard_{g_log_mutex};

  const auto filename{strrchr(file, PATH_SEPARATOR)};

  printf("[%s:%zu] %s\n", filename == nullptr ? file : filename + 1, line,
         text);
}

void topgg::_logf(const char* file, const size_t line, const char* format,
                  ...) {
  std::lock_guard guard_{g_log_mutex};

  const auto filename{strrchr(file, PATH_SEPARATOR)};

  va_list args;
  va_start(args, format);

  printf("[%s:%zu] ", filename == nullptr ? file : filename + 1, line);
  vprintf(format, args);
  putchar('\n');

  va_end(args);
}

void topgg::_elog(const char* file, const size_t line, const char* text) {
  std::lock_guard guard_{g_log_mutex};

  const auto filename{strrchr(file, PATH_SEPARATOR)};

  fprintf(stderr, "[%s:%zu] ERROR: %s\n",
          filename == nullptr ? file : filename + 1, line, text);
}

void topgg::_elogf(const char* file, const size_t line, const char* format,
                   ...) {
  std::lock_guard guard_{g_log_mutex};

  const auto filename{strrchr(file, PATH_SEPARATOR)};

  va_list args;
  va_start(args, format);

  printf("[%s:%zu] ERROR: ", filename == nullptr ? file : filename + 1, line);
  vfprintf(stderr, format, args);
  putchar('\n');

  va_end(args);
}