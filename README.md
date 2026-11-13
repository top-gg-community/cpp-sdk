# Top.gg C++ SDK

The community-maintained C++17 library for Top.gg.

## Chapters

- [Building from source](#building-from-source)
- [Usage](#usage)
  - [General API calls](#general-api-calls)
  - [cpp-httplib Webhooks](#cpp-httplib-webhooks)
  - [cpp-httplib Oauth2](#cpp-httplib-oauth2)

## Building from source

First, clone the git repository like so:

```sh
$ git clone https://github.com/Top-gg-Community/cpp-sdk --depth 1
$ cd cpp-sdk
$ git submodule update --init --recursive
```

The SDK's CMake options is as follows:

| Option name                       | Description                                           | Default  |
| --------------------------------- | ----------------------------------------------------- | -------- |
| `BUILD_SHARED_LIBS`               | Build shared libraries.                               | `ON`     |
| `TOPGG_WEBHOOKS`                  | Enable webhooks (`topgg::webhooks`) support.          | `OFF`    |
| `TOPGG_PROJECT_TOKENS_ONLY`       | Only include project token support.                   | `OFF`    |
| `TOPGG_OAUTH2_ACCESS_TOKENS_ONLY` | Only include oauth2 access token support.             | `OFF`    |
| `TOPGG_TOKEN_EXPIRY_INTERVAL`     | Access token expiry interval for development testing. | `604800` |
| `TOPGG_SSLKEYLOG`                 | SSL keylog file for network debugging with Wireshark. | *Empty*  |
### Linux (Ubuntu or Debian)

```sh
$ sudo apt install libssl-dev libnghttp2-dev libuv1-dev
$ cmake -B build .
$ cmake --build build --config Release
```
### Linux (Fedora)

```sh
$ sudo dnf install openssl-devel libnghttp2-devel libuv-devel
$ cmake -B build .
$ cmake --build build --config Release
```
### Linux (Arch)

```sh
$ sudo pacman -S openssl nghttp2 libuv
$ cmake -B build .
$ cmake --build build --config Release
```
### macOS

```sh
$ brew install openssl nghttp2 libuv
$ cmake -B build .
$ cmake --build build --config Release
```
### Windows
```bat
> vcpkg install openssl nghttp2 libuv
> cmake -B build .
> cmake --build build --config Release
```
## Usage
### General API calls
```cpp
#include <topgg/topgg.h>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <ctime>


int main() {
  try {
    const auto token{std::getenv("TOPGG_TOKEN")};

    topgg::client dbl{token};

    dbl.get_project([](const topgg::result<topgg::project>& result) {
      try {
        const auto& project{result.get()};

        std::cout << "id: " << project.id << std::endl;
        std::cout << "name: " << project.name << std::endl;
        std::cout << "platform: " << project.platform << std::endl;
        std::cout << "type: " << project.type << std::endl;
        std::cout << "headline: " << project.headline << std::endl;
        std::cout << "tags: ";

        for (const auto& tag: project.tags) {
          std::cout << tag << ' ';
        }

        std::cout << std::endl << "current_votes: " << project.current_votes << std::endl;
        std::cout << "total_votes: " << project.total_votes << std::endl;
        std::cout << "review_count: " << project.review_count << std::endl;
        std::cout << "review_score: " << project.review_score << std::endl;
      } catch (const std::exception& error) {
        std::cerr << "get_project error: " << error.what() << std::endl;
      }
    });

    dbl.get_votes(time(nullptr) - 3600, [](const topgg::paginated_result<topgg::vote>& result) {
      try {
        const auto& votes{result.get()};

        for (const auto& vote: votes) {
          std::cout << "user_id: " << vote.user_id << std::endl;
          std::cout << "platform_id: " << vote.platform_id << std::endl;
          std::cout << "weight: " << vote.weight << std::endl;
          std::cout << "created_at: " << vote.created_at << std::endl;
          std::cout << "expires_at: " << vote.expires_at << std::endl;
        }
      } catch (const std::exception& error) {
        std::cerr << "get_votes error: " << error.what() << std::endl;
      }
    });

    dbl.post_announcement("My project announcement", "Lorem ipsum...", topgg::announcement_category::ac_announcement, [](const topgg::empty_result& result) {
      try {
        result.check();
      } catch (const std::exception& error) {
        std::cerr << "post_announcement error: " << error.what() << std::endl;
      }
    });

    dbl.post_commands("[{\"id\":\"1\",\"type\":1,\"application_id\":\"1\",\"name\":\"test\",\"description\":\"command description\",\"default_member_permissions\":\"\",\"version\":\"1\"}]", [](const topgg::empty_result& result) {
      try {
        result.check();
      } catch (const std::exception& error) {
        std::cerr << "post_commands error: " << error.what() << std::endl;
      }
    });

    const auto metrics{topgg::discord_bot_metrics::server_count(15)};

    dbl.post_metrics(static_cast<const topgg::metrics&>(metrics), [](const topgg::empty_result& result) {
      try {
        result.check();
      } catch (const std::exception& error) {
        std::cerr << "simple post_metrics error: " << error.what() << std::endl;
      }
    });

    topgg::locale_map headline{};

    headline.set(topgg::locale::l_english, "hello this is a headline");
    headline.set(topgg::locale::l_german, "hallo das ist eine Überschrift");

    topgg::locale_map page_content{};

    page_content.set(topgg::locale::l_english, "hello this is a page content");
    page_content.set(topgg::locale::l_german, "hallo dies ist ein seiteninhalt");

    dbl.edit_project(headline, page_content, [](const topgg::empty_result& result) {
      try {
        result.check();
      } catch (const std::exception& error) {
        std::cerr << "edit_project error: " << error.what() << std::endl;
      }
    });

    topgg::timestamped_metrics<topgg::discord_bot_metrics> timestamped_metrics{};

    timestamped_metrics.add(1790693201, topgg::discord_bot_metrics::server_count(10));
    timestamped_metrics.add(1790694201, topgg::discord_bot_metrics::server_count(11));
    timestamped_metrics.add(1790695201, topgg::discord_bot_metrics::server_count(12));
    timestamped_metrics.add(1790696201, topgg::discord_bot_metrics::server_count(13));

    dbl.post_metrics(timestamped_metrics, [](const topgg::empty_result& result) {
      try {
        result.check();
      } catch (const std::exception& error) {
        std::cerr << "timestamped post_metrics error: " << error.what() << std::endl;
      }
    });
  } catch (const std::exception& error) {
    std::cerr << "API call error: " << error.what() << std::endl;
  }

  return 0;
}
```
### cpp-httplib Webhooks
```cpp
#include <topgg/topgg.h>
#include <stdexcept>
#include <httplib.h>
#include <iostream>
#include <utility>
#include <memory>


int main() {
  httplib::Server server{};

  auto verifier{std::make_shared<topgg::webhooks::verifier>()};

  server.Post("/webhooks", [verifier = std::shared_ptr{verifier}](const httplib::Request& request, httplib::Response& response) {
    const auto webhooks_secret{std::getenv("TOPGG_WEBHOOKS_SECRET")};  

    if (!request.has_header("x-topgg-signature") || !verifier->verify(webhooks_secret, request.get_header_value("x-topgg-signature"), request.body)) {
      response.status = 401;
      response.set_content("Unauthorized", "text/html");
    } else {
      try {
        const auto payload{topgg::webhooks::parse(request.body)};

        if (std::holds_alternative<topgg::webhooks::vote_create>(payload)) {
          const auto& vote_create{std::get<topgg::webhooks::vote_create>(payload)};

          std::cout << "got vote.create" << std::endl;
          std::cout << "id: " << vote_create.id << std::endl;
          std::cout << "created_at: " << vote_create.created_at << std::endl;
          std::cout << "expires_at: " << vote_create.expires_at << std::endl;
          std::cout << "weight: " << vote_create.weight << std::endl;

          std::cout << "user.id: " << vote_create.user.id << std::endl;
          std::cout << "user.platform_id: " << vote_create.user.platform_id << std::endl;
          std::cout << "user.name: " << vote_create.user.name << std::endl;
          std::cout << "user.avatar: " << vote_create.user.avatar << std::endl;

          std::cout << "project.id: " << vote_create.project.id << std::endl;
          std::cout << "project.platform_id: " << vote_create.project.platform_id << std::endl;
        } else if (std::holds_alternative<topgg::webhooks::test>(payload)) {
          const auto& test{std::get<topgg::webhooks::test>(payload)};

          std::cout << "got webhook.test" << std::endl;
          std::cout << "user.id: " << test.user.id << std::endl;
          std::cout << "user.platform_id: " << test.user.platform_id << std::endl;
          std::cout << "user.name: " << test.user.name << std::endl;
          std::cout << "user.avatar: " << test.user.avatar << std::endl;

          std::cout << "project.id: " << test.project.id << std::endl;
          std::cout << "project.platform_id: " << test.project.platform_id << std::endl;
        }

        response.status = 200;
        response.set_content("OK", "text/html");
      } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << std::endl;

        response.status = 422;
        response.set_content("Unprocessable Content", "text/html");
      }
    }
  });

  server.listen("127.0.0.1", 3000);

  return 0;
}
```
### cpp-httplib Oauth2
```cpp
#include <topgg/topgg.h>
#include <stdexcept>
#include <httplib.h>
#include <iostream>
#include <utility>
#include <chrono>
#include <memory>
#include <thread>


int main() {
  httplib::Server server{};

  const auto client_id{std::getenv("TOPGG_CLIENT_ID")};
  const auto client_secret{std::getenv("TOPGG_CLIENT_SECRET")};

  topgg::oauth2 oauth2{client_id, client_secret, "http://localhost:3000/topgg/oauth2", {
    "user.identify",
    "user.projects.read",
    "user.projects.write",
    "project.information.read",
    "project.information.write",
    "project.votes.read",
    "project.metrics.write",
    "project.announcements.write",
    "project.webhooks.read",
    "project.webhooks.write",
    "project.integrations.read",
    "project.integrations.write"
  }};

  auto oauth2_url{oauth2.new_url()};

  server.Get("/topgg/oauth2", [&server, &oauth2, oauth2_url = std::shared_ptr{oauth2_url}](const httplib::Request& request, httplib::Response& response) {
    if (request.has_param("code") && request.has_param("state")) {
      try {
        oauth2_url->exchange(request.get_param_value("code"), [&server, &oauth2](const topgg::result<topgg::oauth2_session>& result) {
          try {
            auto& session{result.get()};
            auto dbl{oauth2.new_client(session)};

            dbl->get_authorized_user([](const topgg::result<topgg::user>& result) {
              try {
                const auto& user{result.get()};

                std::cout << "id: " << user.id << std::endl;
                std::cout << "username: " << user.username << std::endl;
                std::cout << "avatar: " << user.avatar.value_or("Unknown") << std::endl;
                std::cout << "connections: " << std::endl;

                for (const auto& connection: user.connections) {
                  std::cout << "  id: " << connection.id << std::endl;
                  std::cout << "  platform: " << connection.platform << std::endl;
                }
              } catch (const std::exception& error) {
                std::cerr << "get_authorized_user error: " << error.what() << std::endl;
              }
            });

            dbl->get_authorized_user_projects([](const topgg::result<std::vector<topgg::user_project>>& result) {
              try {
                const auto& projects{result.get()};

                for (const auto& project: projects) {
                  std::cout << "id: " << project.id << std::endl;
                  std::cout << "platform_id: " << project.platform_id << std::endl;
                  std::cout << "name: " << project.name << std::endl;
                  std::cout << "platform: " << project.platform << std::endl;
                  std::cout << "type: " << project.type << std::endl;
                  std::cout << "headline: " << project.headline << std::endl;
                  std::cout << "review_status: " << project.review_status << std::endl;
                }
              } catch (const std::exception& error) {
                std::cerr << "get_authorized_user_projects error: " << error.what() << std::endl;
              }
            });
          } catch (const std::exception& error) {
            std::cerr << "API call error: " << error.what() << std::endl;
          }

          std::this_thread::sleep_for(std::chrono::seconds(10));

          server.stop();
        });

        response.status = 200;
        response.set_content("OK", "text/html");

        return;
      } catch (const std::exception&) {}
    }

    response.status = 401;
    response.set_content("Unauthorized", "text/html");
  });

  std::cout << "Please open this URL:\n\n" << oauth2_url->url << std::endl;

  server.listen("127.0.0.1", 3000);

  return 0;
}
```
