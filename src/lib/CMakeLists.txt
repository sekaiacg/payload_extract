include(libbz2.cmake)
include(libprotobuf-cpp-full.cmake)

# XZ
set(BUILD_TESTING OFF)
set(HAVE_GETOPT_LONG OFF)
set(CREATE_XZ_SYMLINKS OFF)
set(CREATE_LZMA_SYMLINKS OFF)
set(XZ_NLS OFF)
set(XZ_TOOL_SCRIPTS OFF)
set(XZ_TOOL_LZMADEC OFF)
set(XZ_TOOL_LZMAINFO OFF)
set(XZ_TOOL_XZDEC OFF)
set(XZ_TOOL_XZ OFF)
add_subdirectory("xz")

# ZSTD
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
set(ZSTD_BUILD_CONTRIB OFF)
set(BUILD_TESTING OFF)
set(ZSTD_MULTITHREAD_SUPPORT ON)
set(DZSTD_LEGACY_SUPPORT ON)
add_subdirectory("zstd/build/cmake" "zstd")

#cpr
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    set(CPR_FORCE_MBEDTLS_BACKEND ON)
    # MbedTls
    include(FetchContent)
    FetchContent_Declare(MbedTLS
        URL https://github.com/Mbed-TLS/mbedtls/releases/download/v3.6.3.1/mbedtls-3.6.3.1.tar.bz2
        USES_TERMINAL_DOWNLOAD TRUE
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(MbedTLS)
    include_directories(${CMAKE_BINARY_DIR}/_deps/mbedtls-build/include)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPR_FORCE_DARWINSSL_BACKEND ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPR_FORCE_WINSSL_BACKEND ON)
endif ()
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_CURL_EXE OFF)
set(CURL_USE_LIBSSH2 OFF)
set(CURL_USE_LIBPSL OFF)
set(USE_LIBIDN2 OFF)
set(USE_NGHTTP2 OFF)
set(CURL_ZLIB OFF CACHE STRING "")
set(BUILD_LIBCURL_DOCS OFF)
add_subdirectory("cpr")

# payload
add_subdirectory(libpayload)
