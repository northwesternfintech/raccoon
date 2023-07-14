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

configure_file(src/config.h.in config.h)

###############################
# XXX DISABLE TESTING FOR NOW #
###############################
set(BUILD_TESTING OFF)

# ---- Declare library ----

add_library(
  raccoon_lib OBJECT
  # Web
  src/web/ws.cpp

  # Utils
  src/logging.cpp

  # Storage
  src/storage/processing.cpp
)

target_include_directories(
  raccoon_lib ${warning_guard}
  PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>"
)

target_compile_features(raccoon_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(raccoon_lib PRIVATE fmt::fmt)

find_package(quill REQUIRED)
target_link_libraries(raccoon_lib PRIVATE quill::quill)

find_package(glaze REQUIRED)
target_link_libraries(raccoon_lib PRIVATE glaze::glaze)

find_package(hiredis REQUIRED)
target_link_libraries(raccoon_lib PRIVATE hiredis::hiredis)

find_package(CURL REQUIRED)
target_link_libraries(raccoon_lib PRIVATE CURL::libcurl)

find_package(libuv REQUIRED)
if (WIN32)
  target_link_libraries(raccoon_lib PRIVATE uv_a)
else()
  target_link_libraries(raccoon_lib PRIVATE uv)
endif()

# ---- Declare executable ----

add_executable(raccoon_exe src/main.cpp)
add_executable(raccoon::exe ALIAS raccoon_exe)

set_property(TARGET raccoon_exe PROPERTY OUTPUT_NAME raccoon)

target_compile_features(raccoon_exe PRIVATE cxx_std_20)

target_link_libraries(raccoon_exe PRIVATE raccoon_lib)
target_link_libraries(raccoon_exe PRIVATE quill::quill)
target_link_libraries(raccoon_exe PRIVATE CURL::libcurl)
target_link_libraries(raccoon_exe PRIVATE glaze::glaze)

if (WIN32)
  target_link_libraries(raccoon_exe PRIVATE uv_a)
else()
  target_link_libraries(raccoon_exe PRIVATE uv)
endif()

find_package(argparse REQUIRED)
target_link_libraries(raccoon_exe PRIVATE argparse::argparse)

# Git version tracking
FetchContent_Declare(cmake_git_version_tracking
  GIT_REPOSITORY https://github.com/andrew-hardin/cmake-git-version-tracking.git
  GIT_TAG 904dbda1336ba4b9a1415a68d5f203f576b696bb
)
FetchContent_MakeAvailable(cmake_git_version_tracking)

target_link_libraries(raccoon_exe PRIVATE
  cmake_git_version_tracking
)

if(MINGW)
  target_link_libraries(CURL::libcurl INTERFACE bcrypt)
endif()

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
