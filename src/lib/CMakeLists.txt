include(libbspatch.cmake)
include(libbz2.cmake)
include(libfec_rs.cmake)
include(libprotobuf-cpp-full.cmake)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# BROTLI
set(BUILD_SHARED_LIBS OFF)
set(BROTLI_BUNDLED_MODE OFF)
add_subdirectory("brotli")

# XZ
set(BUILD_TESTING OFF)
set(XZ_NLS OFF)
set(XZ_TOOL_XZ OFF)
set(XZ_TOOL_LZMADEC OFF)
set(XZ_TOOL_LZMAINFO OFF)
set(XZ_TOOL_XZDEC OFF)
add_subdirectory("xz")

# ZSTD
set(BUILD_TESTING OFF)
set(ZSTD_BUILD_CONTRIB OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
set(ZSTD_BUILD_SHARED OFF)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_MULTITHREAD_SUPPORT ON)
set(DZSTD_LEGACY_SUPPORT ON)
add_subdirectory("zstd/build/cmake" "zstd")

# WolfSSL
if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin|Windows")
    include(libwolfssl.cmake)
    set(USE_LIB_WOLFSSL ON)
endif ()

# cpr
if (ENABLE_HTTP_CPR)
    include(libcpr.cmake)
endif ()

# libfiles
add_subdirectory(libfiles)
# payload
add_subdirectory(libpayload)
