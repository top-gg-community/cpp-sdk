cmake_minimum_required(VERSION 3.10)

project(
  topgg
  LANGUAGES CXX
  HOMEPAGE_URL "https://docs.top.gg/docs"
  DESCRIPTION "The community-maintained C++17 SDK for Top.gg."
)

get_property(_has_shared GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
set(TOPGG_SSLKEYLOG "" CACHE STRING "SSL keylog file for network debugging with Wireshark")
set(TOPGG_TOKEN_EXPIRY_INTERVAL "604800" CACHE STRING "Access token expiry interval for development testing")
option(TOPGG_PROJECT_TOKENS_ONLY "Only include project token support")
option(TOPGG_OAUTH2_ACCESS_TOKENS_ONLY "Only include oauth2 access token support")
option(TOPGG_WEBHOOKS "Webhooks support")
option(BUILD_SHARED_LIBS "Build shared libraries" ${_has_shared})

file(GLOB TOPGG_SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

if(BUILD_SHARED_LIBS)
add_library(topgg SHARED ${TOPGG_SOURCE_FILES})

if(WIN32)
target_sources(topgg PRIVATE ${CMAKE_CURRENT_LIST_DIR}/topgg.rc)
endif()
else()
add_library(topgg STATIC ${TOPGG_SOURCE_FILES})
endif()

set_target_properties(topgg PROPERTIES
  OUTPUT_NAME           topgg
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED ON
)

if(MSVC)
target_compile_options(topgg PRIVATE /EHsc /permissive- $<$<CONFIG:Debug>:/diagnostics:caret /MDd> $<$<CONFIG:Release>:/MD /O2 /Oi /Oy /Gy>)
else()
target_compile_options(topgg PRIVATE $<$<CONFIG:Release>:-O3> -Wall -Wextra -Wpedantic -Wformat=2 -Wnull-dereference -Wuninitialized -Wdeprecated)
endif()

if(WIN32)
target_compile_definitions(topgg PRIVATE WIN32_LEAN_AND_MEAN)

if(MSVC)
target_compile_definitions(topgg PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
target_compile_definitions(topgg PRIVATE _DEBUG)

if(NOT "${TOPGG_SSLKEYLOG}" STREQUAL "")
target_compile_definitions(topgg PRIVATE TOPGG_SSLKEYLOG="${TOPGG_SSLKEYLOG}")
endif()
endif()

target_compile_definitions(topgg PRIVATE TOPGG_TOKEN_EXPIRY_INTERVAL=${TOPGG_TOKEN_EXPIRY_INTERVAL})

if(TOPGG_PROJECT_TOKENS_ONLY)
target_compile_definitions(topgg PUBLIC TOPGG_PROJECT_TOKENS_ONLY=1)
endif()

if(TOPGG_OAUTH2_ACCESS_TOKENS_ONLY)
target_compile_definitions(topgg PUBLIC TOPGG_OAUTH2_ACCESS_TOKENS_ONLY=1)
endif()

if(TOPGG_WEBHOOKS)
target_compile_definitions(topgg PUBLIC TOPGG_WEBHOOKS=1)
endif()

find_library(OPENSSL_LIBRARY REQUIRED NAMES ssl)
find_library(CRYPTO_LIBRARY REQUIRED NAMES crypto)
find_path(OPENSSL_INCLUDE_DIR NAMES openssl/ssl.h HINTS include)

find_library(LIBUV_LIBRARY REQUIRED NAMES uv)
find_path(LIBUV_INCLUDE_DIR NAMES uv.h HINTS include)

find_library(NGHTTP_LIBRARY REQUIRED NAMES nghttp2)
find_path(NGHTTP_INCLUDE_DIR NAMES nghttp2/nghttp2.h HINTS include)

target_include_directories(topgg PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${OPENSSL_INCLUDE_DIR} ${LIBUV_INCLUDE_DIR} ${NGHTTP_INCLUDE_DIR})
target_link_libraries(topgg PRIVATE ${OPENSSL_LIBRARY} ${CRYPTO_LIBRARY} ${LIBUV_LIBRARY} ${NGHTTP_LIBRARY})

if(WIN32)
target_link_libraries(topgg PRIVATE ws2_32)
endif()