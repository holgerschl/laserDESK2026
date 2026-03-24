cmake_minimum_required(VERSION 3.16)
project(laserdesk_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.18.3
  GIT_SHALLOW    TRUE
)
# Avoid requiring OpenSSL when building as a dependency (MVP uses plain HTTP only).
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-30-2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib nlohmann_json)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

add_library(laserdesk_rtc_core STATIC
  src/dxf/ascii_dxf_lines.cpp
  src/rtc/job_id.cpp
  src/rtc/job/scene_v1.cpp
  src/rtc/job/rtc_job_plan.cpp
  src/rtc/job/dxf_rif_list_mapper.cpp
  src/rtc/mock_rtc_client.cpp
  src/rtc/ethernet_rtc_client.cpp
  src/rtc/rtc_discover.cpp
  src/rtc/rif/telegram_raw.cpp
  src/rtc/rif/udp_channel.cpp
  src/http/api_router.cpp
)
target_include_directories(laserdesk_rtc_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${asio_SOURCE_DIR}/asio/include
)
target_link_libraries(laserdesk_rtc_core PUBLIC nlohmann_json::nlohmann_json httplib::httplib)
target_compile_definitions(laserdesk_rtc_core PUBLIC ASIO_STANDALONE)
if(WIN32)
  target_link_libraries(laserdesk_rtc_core PUBLIC ws2_32)
  target_compile_definitions(laserdesk_rtc_core PUBLIC _WIN32_WINNT=0x0601)
endif()

add_executable(laserdesk_backend src/main.cpp)
target_link_libraries(laserdesk_backend PRIVATE laserdesk_rtc_core)
if(WIN32)
  # VERSIONINFO on all Windows compilers. RT_MANIFEST only on MinGW: MSVC already embeds a manifest;
  # embedding the same via .rc causes CVT1100 duplicate MANIFEST at link time.
  target_sources(laserdesk_backend PRIVATE win/laserdesk_backend.rc)
  if(MINGW)
    target_sources(laserdesk_backend PRIVATE win/laserdesk_manifest_only.rc)
  endif()
endif()

# --- tests ---
enable_testing()
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(laserdesk_tests
  tests/test_mock_rtc.cpp
  tests/test_backend_session.cpp
  tests/test_backend_session_dxf.cpp
  tests/test_dxf_parse.cpp
  tests/test_dxf_rif_list_mapper.cpp
  tests/test_scene_v1.cpp
  tests/test_telegram_raw.cpp
  tests/test_rtc_discover.cpp
  tests/test_ethernet_integration.cpp
)
target_link_libraries(laserdesk_tests PRIVATE laserdesk_rtc_core GTest::gtest_main)
target_compile_definitions(laserdesk_tests PRIVATE
  LASERDESK_TEST_DXF_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/../demo/dxf/SCANLABLogo.dxf")
include(GoogleTest)
gtest_discover_tests(laserdesk_tests)
