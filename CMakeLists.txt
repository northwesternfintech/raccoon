cmake_minimum_required(VERSION 3.14)

include(FetchContent)
include(cmake/prelude.cmake)

project(
    raccoon
    VERSION 0.1.0
    DESCRIPTION "Data fetching and orderbook processing."
    HOMEPAGE_URL "https://github.com/northwesternfintech/raccoon"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/functions.cmake)

configure_file(src/config.h.in config.h)

# ---- Load Dependencies ----

# Conan
find_package(fmt REQUIRED)       # String formatting
find_package(quill REQUIRED)     # Logging

find_package(glaze REQUIRED)     # JSON parsing
find_package(hiredis REQUIRED)   # Redis client
find_package(libuv REQUIRED)     # Event loop

find_package(argparse REQUIRED)  # Argument parser

# Vendored/submodule
add_subdirectory(3rd-party)

# Git version tracking
FetchContent_Declare(cmake_git_version_tracking
  GIT_REPOSITORY https://github.com/andrew-hardin/cmake-git-version-tracking.git
  GIT_TAG 9b5fc5088b4089ff2adc20d607976b9923e3d737
)
FetchContent_MakeAvailable(cmake_git_version_tracking)

# ---- Declare library ----

add_library(
  raccoon_lib OBJECT
  # Web
  src/web/ws.cpp
  src/web/manager.cpp

  # Utils
  src/logging.cpp

  # Storage
  src/storage/processing.cpp
  src/storage/orderbook.cpp
  src/storage/trades.cpp
)

target_include_directories(
  raccoon_lib ${warning_guard}
  PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>"
)

target_compile_features(raccoon_lib PUBLIC cxx_std_20)

target_link_libraries(raccoon_lib PRIVATE fmt::fmt)
target_link_libraries(raccoon_lib PRIVATE quill::quill)
target_link_libraries(raccoon_lib PRIVATE uv)
target_link_libraries(raccoon_lib PRIVATE glaze::glaze)
target_link_libraries(raccoon_lib PRIVATE hiredis::hiredis)
target_link_libraries_system(raccoon_lib PRIVATE CURL::libcurl)


# ---- Declare executable ----

add_executable(raccoon_exe src/main.cpp)
add_executable(raccoon::exe ALIAS raccoon_exe)

set_property(TARGET raccoon_exe PROPERTY OUTPUT_NAME raccoon)

target_compile_features(raccoon_exe PRIVATE cxx_std_20)

target_link_libraries(raccoon_exe PRIVATE raccoon_lib)
target_link_libraries(raccoon_exe PRIVATE quill::quill)
target_link_libraries(raccoon_exe PRIVATE uv)
target_link_libraries(raccoon_exe PRIVATE glaze::glaze)
target_link_libraries(raccoon_exe PRIVATE hiredis::hiredis)
target_link_libraries_system(raccoon_exe PRIVATE CURL::libcurl)

target_link_libraries(raccoon_exe PRIVATE argparse::argparse)
target_link_libraries(raccoon_exe PRIVATE cmake_git_version_tracking)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT raccoon_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
    AUTHOR_WARNING
    "Developer mode is intended for developers of raccoon"
  )
endif()

include(cmake/dev-mode.cmake)
