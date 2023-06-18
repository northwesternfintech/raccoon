cmake_minimum_required(VERSION 3.14)

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

# ---- Declare library ----

add_library(
    raccoon_lib OBJECT
    src/lib.cpp
    # Utils
    src/logging.cpp
)

target_include_directories(
    raccoon_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(raccoon_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(raccoon_lib PRIVATE fmt::fmt)

find_package(quill REQUIRED)
target_link_libraries(raccoon_lib PRIVATE quill::quill)

# ---- Declare executable ----

add_executable(raccoon_exe src/main.cpp)
add_executable(raccoon::exe ALIAS raccoon_exe)

set_property(TARGET raccoon_exe PROPERTY OUTPUT_NAME raccoon)

target_compile_features(raccoon_exe PRIVATE cxx_std_20)

target_link_libraries(raccoon_exe PRIVATE raccoon_lib)
target_link_libraries(raccoon_exe PRIVATE quill::quill)

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
