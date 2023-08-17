cmake_minimum_required(VERSION 3.14)

block()
  # No linters
  set(CMAKE_CXX_CLANG_TIDY "")
  set(CMAKE_CXX_CPPCHECK "")

  # Extensions on
  set(CMAKE_CXX_EXTENSIONS ON)

  ##
  ## LIBCURL
  ##
  block()
    # Turn on curl debug for debug builds
    if(CMAKE_BUILD_TYPE MATCHES DEBUG)
      set(ENABLE_DEBUG ON)
    endif()

    # Use SChannel on Win32
    if(WIN32)
      set(CURL_USE_SCHANNEL ON)
    endif()

    # Turn on Websockets
    set(ENABLE_WEBSOCKETS ON)

    # Use ZLIB
    set(CURL_ZLIB ON)

    # Statically link
    set(BUILD_SHARED_LIBS OFF)

    # Build curl
    add_subdirectory(curl)

    # Fix issue with curl on win32
    if(MINGW)
      target_link_libraries(libcurl INTERFACE bcrypt)
    endif()
  endblock()
endblock()
